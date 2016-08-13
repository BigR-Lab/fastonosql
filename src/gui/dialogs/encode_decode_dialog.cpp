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

#include "gui/dialogs/encode_decode_dialog.h"

#include <string>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSplitter>
#include <QToolButton>

#include "common/text_decoders/iedcoder.h"
#include "common/convert2string.h"
#include "common/qt/convert2string.h"

#include "translations/global.h"

#include "gui/gui_factory.h"
#include "gui/fasto_editor.h"

namespace fastonosql {
namespace gui {

EncodeDecodeDialog::EncodeDecodeDialog(QWidget* parent)
  : QDialog(parent) {
  setWindowIcon(GuiFactory::instance().encodeDecodeIcon());

  setWindowTitle(translations::trEncodeDecode);
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  QVBoxLayout* layout = new QVBoxLayout;

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  QPushButton* closeButton = buttonBox->button(QDialogButtonBox::Close);
  buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
  VERIFY(connect(buttonBox, &QDialogButtonBox::rejected, this, &EncodeDecodeDialog::reject));

  QToolButton* decode = new QToolButton;
  decode->setIcon(GuiFactory::instance().executeIcon());
  VERIFY(connect(decode, &QToolButton::clicked, this, &EncodeDecodeDialog::decodeOrEncode));

  decoders_ = new QComboBox;
  for (size_t i = 0; i < SIZEOFMASS(common::EDecoderTypes); ++i) {
    std::string estr = common::EDecoderTypes[i];
    common::EDTypes etype = common::ConvertFromString<common::EDTypes>(estr);
    decoders_->addItem(common::ConvertFromString<QString>(estr), etype);
  }

  QHBoxLayout* toolBarLayout = new QHBoxLayout;
  toolBarLayout->addWidget(decode);
  toolBarLayout->addWidget(decoders_);

  encodeButton_ = new QRadioButton;
  decodeButton_ = new QRadioButton;
  toolBarLayout->addWidget(encodeButton_);
  toolBarLayout->addWidget(decodeButton_);

  QSplitter* splitter = new QSplitter;
  splitter->setOrientation(Qt::Horizontal);
  splitter->setHandleWidth(1);
  toolBarLayout->addWidget(splitter);

  input_ = new FastoEditor;
  output_ = new FastoEditor;

  layout->addWidget(input_);
  layout->addLayout(toolBarLayout);
  layout->addWidget(output_);
  layout->addWidget(buttonBox);

  setMinimumSize(QSize(min_width, min_height));
  setLayout(layout);

  retranslateUi();
}

void EncodeDecodeDialog::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }

  QWidget::changeEvent(e);
}

void EncodeDecodeDialog::decodeOrEncode() {
  QString in = input_->text();
  if (in.isEmpty()) {
    return;
  }

  output_->clear();
  QVariant var = decoders_->currentData();
  common::EDTypes currentType = static_cast<common::EDTypes>(qvariant_cast<unsigned char>(var));
  common::IEDcoder* dec = common::IEDcoder::createEDCoder(currentType);
  CHECK(dec);

  std::string sin = common::ConvertToString(in);
  std::string out;
  common::Error er = encodeButton_->isChecked() ? dec->encode(sin, &out) : dec->decode(sin, &out);
  if (!er) {
    output_->setText(common::ConvertFromString<QString>(out));
  }
  delete dec;
}

void EncodeDecodeDialog::retranslateUi() {
  encodeButton_->setText(translations::trEncode);
  decodeButton_->setText(translations::trDecode);
}

}  // namespace gui
}  // namespace fastonosql
