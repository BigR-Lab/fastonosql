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

#include "gui/dialogs/history_server_dialog.h"

#include <stddef.h>                     // for size_t
#include <stdint.h>                     // for uint32_t

#include <memory>                       // for __shared_ptr
#include <utility>                      // for make_pair
#include <vector>                       // for vector

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>

#include "common/convert2string.h"      // for ConvertFromString
#include "common/error.h"               // for Error
#include "common/macros.h"              // for VERIFY, UNUSED, CHECK
#include "common/value.h"               // for ErrorValue, Value

#include "core/iserver.h"               // for IServer
#include "core/types.h"                 // for Field, info_field_t, etc

#include "fasto/qt/gui/base/graph_widget.h"  // for GraphWidget, etc
#include "fasto/qt/gui/glass_widget.h"  // for GlassWidget

#include "gui/gui_factory.h"            // for GuiFactory

#include "translations/global.h"        // for trClearHistory, trLoading

namespace {
  const QString trHistoryTemplate_1S = QObject::tr("%1 history");
}

namespace fastonosql {
namespace gui {

ServerHistoryDialog::ServerHistoryDialog(core::IServerSPtr server, QWidget* parent)
  : QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint ), server_(server) {
  CHECK(server_);

  setWindowIcon(GuiFactory::instance().icon(server_->type()));

  graphWidget_ = new fasto::qt::gui::GraphWidget;
  settingsGraph_ = new QWidget;
  QHBoxLayout* mainL = new QHBoxLayout;

  QSplitter* splitter = new QSplitter;
  splitter->setOrientation(Qt::Horizontal);
  splitter->setHandleWidth(1);

  mainL->addWidget(splitter);
  splitter->addWidget(settingsGraph_);

  clearHistory_ = new QPushButton;
  VERIFY(connect(clearHistory_, &QPushButton::clicked, this, &ServerHistoryDialog::clearHistory));
  serverInfoGroupsNames_ = new QComboBox;
  serverInfoFields_ = new QComboBox;

  typedef void (QComboBox::*curc)(int);
  VERIFY(connect(serverInfoGroupsNames_, static_cast<curc>(&QComboBox::currentIndexChanged),
                 this, &ServerHistoryDialog::refreshInfoFields));
  VERIFY(connect(serverInfoFields_, static_cast<curc>(&QComboBox::currentIndexChanged),
                 this, &ServerHistoryDialog::refreshGraph));

  const auto fields = core::infoFieldsFromType(server_->type());
  for (size_t i = 0; i < fields.size(); ++i) {
    core::info_field_t field = fields[i];
    serverInfoGroupsNames_->addItem(common::ConvertFromString<QString>(field.first));
  }
  QVBoxLayout* setingsLayout = new QVBoxLayout;
  setingsLayout->addWidget(clearHistory_);
  setingsLayout->addWidget(serverInfoGroupsNames_);
  setingsLayout->addWidget(serverInfoFields_);
  settingsGraph_->setLayout(setingsLayout);

  splitter->addWidget(graphWidget_);
  setMinimumSize(QSize(min_width, min_height));
  setLayout(mainL);

  glassWidget_ = new fasto::qt::gui::GlassWidget(GuiFactory::instance().pathToLoadingGif(),
                                                 translations::trLoading, 0.5,
                                                 QColor(111, 111, 100), this);
  VERIFY(connect(server.get(), &core::IServer::startedLoadServerHistoryInfo,
                 this, &ServerHistoryDialog::startLoadServerHistoryInfo));
  VERIFY(connect(server.get(), &core::IServer::finishedLoadServerHistoryInfo,
                 this, &ServerHistoryDialog::finishLoadServerHistoryInfo));
  VERIFY(connect(server.get(), &core::IServer::startedClearServerHistory,
                 this, &ServerHistoryDialog::startClearServerHistory));
  VERIFY(connect(server.get(), &core::IServer::finishedClearServerHistory,
                 this, &ServerHistoryDialog::finishClearServerHistory));
  VERIFY(connect(server.get(), &core::IServer::serverInfoSnapShoot,
                 this, &ServerHistoryDialog::snapShotAdd));
  retranslateUi();
}

void ServerHistoryDialog::startLoadServerHistoryInfo(const core::events_info::ServerInfoHistoryRequest& req) {
  UNUSED(req);

  glassWidget_->start();
}

void ServerHistoryDialog::finishLoadServerHistoryInfo(const core::events_info::ServerInfoHistoryResponce& res) {
  glassWidget_->stop();
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  infos_ = res.infos();
  reset();
}

void ServerHistoryDialog::startClearServerHistory(const core::events_info::ClearServerHistoryRequest& req) {

  UNUSED(req);
}

void ServerHistoryDialog::finishClearServerHistory(const core::events_info::ClearServerHistoryResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  requestHistoryInfo();
}

void ServerHistoryDialog::snapShotAdd(core::ServerInfoSnapShoot snapshot) {
  infos_.push_back(snapshot);
  reset();
}

void ServerHistoryDialog::clearHistory() {
  core::events_info::ClearServerHistoryRequest req(this);
  server_->clearHistory(req);
}

void ServerHistoryDialog::refreshInfoFields(int index) {
  if (index == -1) {
    return;
  }

  serverInfoFields_->clear();

  std::vector<core::info_field_t> fields = infoFieldsFromType(server_->type());
  std::vector<core::Field> field = fields[index].second;
  for (uint32_t i = 0; i < field.size(); ++i) {
    core::Field fl = field[i];
    if (fl.isIntegral()) {
      serverInfoFields_->addItem(common::ConvertFromString<QString>(fl.name), i);
    }
  }
}

void ServerHistoryDialog::refreshGraph(int index) {
  if (index == -1) {
    return;
  }

  unsigned char serverIndex = serverInfoGroupsNames_->currentIndex();
  QVariant var = serverInfoFields_->itemData(index);
  unsigned char indexIn = qvariant_cast<unsigned char>(var);
  fasto::qt::gui::GraphWidget::nodes_container_type nodes;
  for (auto it = infos_.begin(); it != infos_.end(); ++it) {
    auto val = *it;
    if (!val.isValid()) {
      continue;
    }

    common::Value* value = val.info->valueByIndexes(serverIndex, indexIn);  // allocate
    if (value) {
      qreal graphY = 0.0f;
      if (value->getAsDouble(&graphY)) {
        nodes.push_back(std::make_pair(val.msec, graphY));
      }
      delete value;
    }
  }

  graphWidget_->setNodes(nodes);
}

void ServerHistoryDialog::changeEvent(QEvent* e) {
  if (e->type() == QEvent::LanguageChange) {
    retranslateUi();
  }
  QDialog::changeEvent(e);
}

void ServerHistoryDialog::showEvent(QShowEvent* e) {
  QDialog::showEvent(e);
  requestHistoryInfo();
}

void ServerHistoryDialog::reset() {
  refreshGraph(serverInfoFields_->currentIndex());
}

void ServerHistoryDialog::retranslateUi() {
  QString name = common::ConvertFromString<QString>(server_->name());
  setWindowTitle(trHistoryTemplate_1S.arg(name));
  clearHistory_->setText(translations::trClearHistory);
}

void ServerHistoryDialog::requestHistoryInfo() {
  core::events_info::ServerInfoHistoryRequest req(this);
  server_->requestHistoryInfo(req);
}

}  // namespace gui
}  // namespace fastonosql
