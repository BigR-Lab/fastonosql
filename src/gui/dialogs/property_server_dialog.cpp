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

#include "gui/dialogs/property_server_dialog.h"

#include <QHBoxLayout>
#include <QTableView>

#include "fasto/qt/gui/glass_widget.h"

#include "core/iserver.h"

#include "translations/global.h"

#include "gui/gui_factory.h"
#include "gui/property_table_model.h"

namespace {
  const QString trPropertiesTemplate_1S = QObject::tr("%1 properties");
}

namespace fastonosql {
namespace gui {

PropertyServerDialog::PropertyServerDialog(core::IServerSPtr server, QWidget* parent)
  : QDialog(parent), server_(server) {
  CHECK(server_);

  setWindowIcon(GuiFactory::instance().icon(server->type()));

  PropertyTableModel* mod = new PropertyTableModel(this);
  propertyes_table_ = new QTableView;
  VERIFY(connect(mod, &PropertyTableModel::changedProperty,
                 this, &PropertyServerDialog::changedProperty));
  propertyes_table_->setModel(mod);

  QHBoxLayout* mainL = new QHBoxLayout;
  mainL->addWidget(propertyes_table_);
  setLayout(mainL);

  glassWidget_ = new fasto::qt::gui::GlassWidget(GuiFactory::instance().pathToLoadingGif(),
                                                 translations::trLoading, 0.5,
                                                 QColor(111, 111, 100), this);

  VERIFY(connect(server.get(), &core::IServer::startedLoadServerProperty,
                 this, &PropertyServerDialog::startServerProperty));
  VERIFY(connect(server.get(), &core::IServer::finishedLoadServerProperty,
                 this, &PropertyServerDialog::finishServerProperty));
  VERIFY(connect(server.get(), &core::IServer::startedChangeServerProperty,
                 this, &PropertyServerDialog::startServerChangeProperty));
  VERIFY(connect(server.get(), &core::IServer::finishedChangeServerProperty,
                 this, &PropertyServerDialog::finishServerChangeProperty));
  retranslateUi();
}

void PropertyServerDialog::startServerProperty(const core::events_info::ServerPropertyInfoRequest& req) {
  glassWidget_->start();
}

void PropertyServerDialog::finishServerProperty(const core::events_info::ServerPropertyInfoResponce& res) {
  glassWidget_->stop();
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  if (server_->type() == core::REDIS) {
    core::ServerPropertyInfo inf = res.info;
    PropertyTableModel* model = qobject_cast<PropertyTableModel*>(propertyes_table_->model());
    for (size_t i = 0; i < inf.propertyes.size(); ++i) {
      core::property_t it = inf.propertyes[i];
      model->insertItem(new PropertyTableItem(it));
    }
  }
}

void PropertyServerDialog::startServerChangeProperty(const core::events_info::ChangeServerPropertyInfoRequest& req) {
}

void PropertyServerDialog::finishServerChangeProperty(const core::events_info::ChangeServerPropertyInfoResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  if (server_->type() == core::REDIS) {
    core::property_t pr = res.new_item;
    if (res.is_change) {
      PropertyTableModel* model = qobject_cast<PropertyTableModel*>(propertyes_table_->model());
      model->changeProperty(pr);
    }
  }
}

void PropertyServerDialog::changedProperty(const core::property_t& prop) {
  core::events_info::ChangeServerPropertyInfoRequest req(this, prop);
  server_->changeProperty(req);
}

void PropertyServerDialog::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }
  QDialog::changeEvent(e);
}

void PropertyServerDialog::showEvent(QShowEvent* e) {
  QDialog::showEvent(e);
  core::events_info::ServerPropertyInfoRequest req(this);
  server_->serverProperty(req);
}

void PropertyServerDialog::retranslateUi() {
  QString name = common::ConvertFromString<QString>(server_->name());
  setWindowTitle(trPropertiesTemplate_1S.arg(name));
}

}  // namespace gui
}  // namespace fastonosql
