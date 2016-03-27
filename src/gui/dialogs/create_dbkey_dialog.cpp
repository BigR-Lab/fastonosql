/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/dialogs/create_dbkey_dialog.h"

#include <vector>
#include <string>

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QGridLayout>
#include <QGroupBox>
#include <QEvent>

#include "common/qt/convert_string.h"

#include "gui/gui_factory.h"
#include "gui/dialogs/input_dialog.h"

#include "translations/global.h"

namespace fastonosql {
namespace gui {

CreateDbKeyDialog::CreateDbKeyDialog(const QString& title, core::connectionTypes type, QWidget* parent)
  : QDialog(parent), type_(type), value_() {
  setWindowIcon(GuiFactory::instance().icon(type_));
  setWindowTitle(title);

  QGridLayout* kvLayout = new QGridLayout;

  kvLayout->addWidget(new QLabel(tr("Type:")), 0, 0);
  typesCombo_ = new QComboBox;
  std::vector<common::Value::Type> types = supportedTypesFromType(type);
  int string_index = 0;
  for (size_t i = 0; i < types.size(); ++i) {
    common::Value::Type t = types[i];
    if (t == common::Value::TYPE_STRING) {
      string_index = i;
    }
    QString type = common::convertFromString<QString>(common::Value::toString(t));
    typesCombo_->addItem(GuiFactory::instance().icon(t), type, t);
  }

  typedef void (QComboBox::*ind)(int);
  VERIFY(connect(typesCombo_, static_cast<ind>(&QComboBox::currentIndexChanged),
                 this, &CreateDbKeyDialog::typeChanged));
  kvLayout->addWidget(typesCombo_, 0, 1);

  // key layout
  kvLayout->addWidget(new QLabel(tr("Key:")), 1, 0);
  keyEdit_ = new QLineEdit;
  kvLayout->addWidget(keyEdit_, 1, 1);

  // value layout
  kvLayout->addWidget(new QLabel(tr("Value:")), 2, 0);
  valueEdit_ = new QLineEdit;
  kvLayout->addWidget(valueEdit_, 2, 1);
  valueEdit_->setVisible(true);

  valueListEdit_ = new QListWidget;
  valueListEdit_->setContextMenuPolicy(Qt::ActionsContextMenu);
  valueListEdit_->setSelectionMode(QAbstractItemView::SingleSelection);
  valueListEdit_->setSelectionBehavior(QAbstractItemView::SelectRows);

  QAction* addItem = new QAction(translations::trAddItem, this);
  VERIFY(connect(addItem, &QAction::triggered, this, &CreateDbKeyDialog::addItem));
  valueListEdit_->addAction(addItem);

  QAction* removeItem = new QAction(translations::trRemoveItem, this);
  VERIFY(connect(removeItem, &QAction::triggered, this, &CreateDbKeyDialog::removeItem));
  valueListEdit_->addAction(removeItem);

  kvLayout->addWidget(valueListEdit_, 2, 1);
  valueListEdit_->setVisible(false);

  valueTableEdit_ = new QTableWidget(0, 2);
  valueTableEdit_->setContextMenuPolicy(Qt::ActionsContextMenu);
  valueTableEdit_->setSelectionBehavior(QAbstractItemView::SelectRows);
  valueTableEdit_->verticalHeader()->hide();
  valueTableEdit_->horizontalHeader()->hide();

  valueTableEdit_->addAction(addItem);
  valueTableEdit_->addAction(removeItem);

  kvLayout->addWidget(valueTableEdit_, 2, 1);
  valueTableEdit_->setVisible(false);

  addItemButton_ = new QPushButton(translations::trAddItem);
  VERIFY(connect(addItemButton_, &QPushButton::clicked, this, &CreateDbKeyDialog::addItem));
  kvLayout->addWidget(addItemButton_, 3, 0);
  addItemButton_->setVisible(false);

  removeItemButton_ = new QPushButton(translations::trRemoveItem);
  VERIFY(connect(removeItemButton_, &QPushButton::clicked, this, &CreateDbKeyDialog::removeItem));
  kvLayout->addWidget(removeItemButton_, 3, 1);
  removeItemButton_->setVisible(false);

  generalBox_ = new QGroupBox;
  generalBox_->setLayout(kvLayout);

  // main layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(generalBox_);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
  buttonBox->setOrientation(Qt::Horizontal);
  buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  VERIFY(connect(buttonBox, &QDialogButtonBox::accepted, this, &CreateDbKeyDialog::accept));
  VERIFY(connect(buttonBox, &QDialogButtonBox::rejected, this, &CreateDbKeyDialog::reject));
  layout->addWidget(buttonBox);

  typesCombo_->setCurrentIndex(string_index);
  setMinimumSize(QSize(min_width, min_height));
  setLayout(layout);
  retranslateUi();
}

core::NDbKValue CreateDbKeyDialog::key() const {
  core::NKey key(common::convertToString(keyEdit_->text()));
  return core::NDbKValue(key, value_);
}

void CreateDbKeyDialog::accept() {
  if (validateAndApply()) {
    QDialog::accept();
  }
}

void CreateDbKeyDialog::typeChanged(int index) {
  QVariant var = typesCombo_->itemData(index);
  common::Value::Type type = (common::Value::Type)qvariant_cast<unsigned char>(var);

  valueEdit_->clear();
  valueTableEdit_->clear();
  valueListEdit_->clear();

  if (type == common::Value::TYPE_ARRAY || type == common::Value::TYPE_SET) {
    valueListEdit_->setVisible(true);
    valueEdit_->setVisible(false);
    valueTableEdit_->setVisible(false);
    addItemButton_->setVisible(true);
    removeItemButton_->setVisible(true);
  } else if (type == common::Value::TYPE_ZSET || type == common::Value::TYPE_HASH) {
    valueTableEdit_->setVisible(true);
    valueEdit_->setVisible(false);
    valueListEdit_->setVisible(false);
    addItemButton_->setVisible(true);
    removeItemButton_->setVisible(true);
  } else {
    valueEdit_->setVisible(true);
    valueListEdit_->setVisible(false);
    valueTableEdit_->setVisible(false);
    addItemButton_->setVisible(false);
    removeItemButton_->setVisible(false);
    if (type == common::Value::TYPE_INTEGER || type == common::Value::TYPE_UINTEGER) {
      valueEdit_->setValidator(new QIntValidator(this));
    } else if (type == common::Value::TYPE_BOOLEAN) {
      QRegExp rx("true|false");
      valueEdit_->setValidator(new QRegExpValidator(rx, this));
    } else if (type == common::Value::TYPE_DOUBLE) {
      valueEdit_->setValidator(new QDoubleValidator(this));
    } else {
      QRegExp rx(".*");
      valueEdit_->setValidator(new QRegExpValidator(rx, this));
    }
  }
}

void CreateDbKeyDialog::addItem() {
  if (valueListEdit_->isVisible()) {
    InputDialog diag(this, translations::trAddItem, InputDialog::SingleLine, translations::trValue);
    int result = diag.exec();
    if (result != QDialog::Accepted) {
      return;
    }

    QString text = diag.firstText();
    if (!text.isEmpty()) {
      QListWidgetItem* nitem = new QListWidgetItem(text, valueListEdit_);
      nitem->setFlags(nitem->flags() | Qt::ItemIsEditable);
      valueListEdit_->addItem(nitem);
    }
  } else if (valueTableEdit_->isVisible()) {
    int index = typesCombo_->currentIndex();
    QVariant var = typesCombo_->itemData(index);
    common::Value::Type t = (common::Value::Type)qvariant_cast<unsigned char>(var);

    InputDialog diag(this, translations::trAddItem, InputDialog::DoubleLine,
                     t == common::Value::TYPE_HASH ? translations::trField :
                                                     translations::trScore, translations::trValue);
    int result = diag.exec();
    if (result != QDialog::Accepted) {
      return;
    }

    QString ftext = diag.firstText();
    QString stext = diag.secondText();

    if (!ftext.isEmpty() && !stext.isEmpty()) {
      QTableWidgetItem* fitem = new QTableWidgetItem(ftext);
      fitem->setFlags(fitem->flags() | Qt::ItemIsEditable);

      QTableWidgetItem* sitem = new QTableWidgetItem(stext);
      sitem->setFlags(sitem->flags() | Qt::ItemIsEditable);

      valueTableEdit_->insertRow(0);
      valueTableEdit_->setItem(0, 0, fitem);
      valueTableEdit_->setItem(0, 1, sitem);
    }
  }
}

void CreateDbKeyDialog::removeItem() {
  if (valueListEdit_->isVisible()) {
    QListWidgetItem* ritem = valueListEdit_->currentItem();
    delete ritem;
  } else if (valueTableEdit_->isVisible()) {
    int row = valueTableEdit_->currentRow();
    valueTableEdit_->removeRow(row);
  }
}

void CreateDbKeyDialog::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }
  QDialog::changeEvent(e);
}

bool CreateDbKeyDialog::validateAndApply() {
  if (keyEdit_->text().isEmpty()) {
    return false;
  }

  common::Value* obj = item();
  if (!obj) {
    return false;
  }

  value_.reset(obj);
  return true;
}

void CreateDbKeyDialog::retranslateUi() {
  generalBox_->setTitle(tr("Key/Value input"));
}

common::Value* CreateDbKeyDialog::item() const {
  int index = typesCombo_->currentIndex();
  QVariant var = typesCombo_->itemData(index);
  common::Value::Type t = (common::Value::Type)qvariant_cast<unsigned char>(var);
  if (t == common::Value::TYPE_ARRAY) {
    if (valueListEdit_->count() == 0) {
      return nullptr;
    }
    common::ArrayValue* ar = common::Value::createArrayValue();
    for (size_t i = 0; i < valueListEdit_->count(); ++i) {
      std::string val = common::convertToString(valueListEdit_->item(i)->text());
      ar->appendString(val);
    }

    return ar;
  } else if (t == common::Value::TYPE_SET) {
    if (valueListEdit_->count() == 0) {
      return nullptr;
    }
    common::SetValue* ar = common::Value::createSetValue();
    for (size_t i = 0; i < valueListEdit_->count(); ++i) {
      std::string val = common::convertToString(valueListEdit_->item(i)->text());
      ar->insert(val);
    }

    return ar;
  } else if (t == common::Value::TYPE_ZSET) {
    if (valueTableEdit_->rowCount() == 0) {
      return nullptr;
    }

    common::ZSetValue* ar = common::Value::createZSetValue();
    for (size_t i = 0; i < valueTableEdit_->rowCount(); ++i) {
      QTableWidgetItem* kitem = valueTableEdit_->item(i, 0);
      QTableWidgetItem* vitem = valueTableEdit_->item(i, 0);

      std::string key = common::convertToString(kitem->text());
      std::string val = common::convertToString(vitem->text());
      ar->insert(key, val);
    }

    return ar;
  } else if (t == common::Value::TYPE_HASH) {
    if (valueTableEdit_->rowCount() == 0) {
      return nullptr;
    }

    common::HashValue* ar = common::Value::createHashValue();
    for (size_t i = 0; i < valueTableEdit_->rowCount(); ++i) {
      QTableWidgetItem* kitem = valueTableEdit_->item(i, 0);
      QTableWidgetItem* vitem = valueTableEdit_->item(i, 0);

      std::string key = common::convertToString(kitem->text());
      std::string val = common::convertToString(vitem->text());
      ar->insert(key, val);
    }

    return ar;
  } else {
    QString text = valueEdit_->text();
    if (text.isEmpty()) {
      return nullptr;
    }

    return common::Value::createStringValue(common::convertToString(text));
  }
}

}  // namespace gui
}  // namespace fastonosql
