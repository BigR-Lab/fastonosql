/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/dialogs/history_server_dialog.h"

#include <string>
#include <vector>

#include <QHBoxLayout>
#include <QSplitter>
#include <QComboBox>
#include <QPushButton>

#include "fasto/qt/gui/base/graph_widget.h"
#include "fasto/qt/gui/glass_widget.h"

#include "core/iserver.h"

#include "gui/gui_factory.h"

#include "translations/global.h"

namespace fastonosql {

ServerHistoryDialog::ServerHistoryDialog(IServerSPtr server, QWidget* parent)
  : QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint ), server_(server) {
  CHECK(server_);

  setWindowIcon(GuiFactory::instance().icon(server_->type()));

  graphWidget_ = new fasto::qt::gui::GraphWidget;
  settingsGraph_ = new QWidget;
  QHBoxLayout *mainL = new QHBoxLayout;

  QSplitter *splitter = new QSplitter;
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

  const std::vector<std::string> headers = infoHeadersFromType(server_->type());
  for (size_t i = 0; i < headers.size(); ++i) {
      serverInfoGroupsNames_->addItem(common::convertFromString<QString>(headers[i]));
  }
  QVBoxLayout *setingsLayout = new QVBoxLayout;
  setingsLayout->addWidget(clearHistory_);
  setingsLayout->addWidget(serverInfoGroupsNames_);
  setingsLayout->addWidget(serverInfoFields_);
  settingsGraph_->setLayout(setingsLayout);

  splitter->addWidget(graphWidget_);
  setLayout(mainL);

  glassWidget_ = new fasto::qt::gui::GlassWidget(GuiFactory::instance().pathToLoadingGif(),
                                                 translations::trLoading, 0.5,
                                                 QColor(111, 111, 100), this);
  VERIFY(connect(server.get(), &IServer::startedLoadServerHistoryInfo,
                 this, &ServerHistoryDialog::startLoadServerHistoryInfo));
  VERIFY(connect(server.get(), &IServer::finishedLoadServerHistoryInfo,
                 this, &ServerHistoryDialog::finishLoadServerHistoryInfo));
  VERIFY(connect(server.get(), &IServer::startedClearServerHistory,
                 this, &ServerHistoryDialog::startClearServerHistory));
  VERIFY(connect(server.get(), &IServer::finishedClearServerHistory,
                 this, &ServerHistoryDialog::finishClearServerHistory));
  VERIFY(connect(server.get(), &IServer::serverInfoSnapShoot,
                 this, &ServerHistoryDialog::snapShotAdd));
  retranslateUi();
}

void ServerHistoryDialog::startLoadServerHistoryInfo(const events_info::ServerInfoHistoryRequest& req) {
  glassWidget_->start();
}

void ServerHistoryDialog::finishLoadServerHistoryInfo(const events_info::ServerInfoHistoryResponce& res) {
  glassWidget_->stop();
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  infos_ = res.infos();
  reset();
}

void ServerHistoryDialog::startClearServerHistory(const events_info::ClearServerHistoryRequest& req) {
}

void ServerHistoryDialog::finishClearServerHistory(const events_info::ClearServerHistoryResponce& res) {
  common::Error er = res.errorInfo();
  if (er && er->isError()) {
    return;
  }

  requestHistoryInfo();
}

void ServerHistoryDialog::snapShotAdd(ServerInfoSnapShoot snapshot) {
  infos_.push_back(snapshot);
  reset();
}

void ServerHistoryDialog::clearHistory() {
  events_info::ClearServerHistoryRequest req(this);
  server_->clearHistory(req);
}

void ServerHistoryDialog::refreshInfoFields(int index) {
  if (index == -1) {
    return;
  }

  serverInfoFields_->clear();

  std::vector< std::vector<Field> > fields = infoFieldsFromType(server_->type());
  std::vector<Field> field = fields[index];
  for (size_t i = 0; i < field.size(); ++i) {
    Field fl = field[i];
    if (fl.isIntegral()) {
      serverInfoFields_->addItem(common::convertFromString<QString>(fl.name), i);
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
  for (events_info::ServerInfoHistoryResponce::infos_container_type::iterator it = infos_.begin();
      it != infos_.end(); ++it) {
    events_info::ServerInfoHistoryResponce::infos_container_type::value_type val = *it;
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
  emit showed();
  requestHistoryInfo();
}

void ServerHistoryDialog::reset() {
  refreshGraph(serverInfoFields_->currentIndex());
}

void ServerHistoryDialog::retranslateUi() {
  setWindowTitle(tr("%1 history").arg(server_->name()));
  clearHistory_->setText(translations::trClearHistory);
}

void ServerHistoryDialog::requestHistoryInfo() {
  events_info::ServerInfoHistoryRequest req(this);
  server_->requestHistoryInfo(req);
}

}  // namespace fastonosql
