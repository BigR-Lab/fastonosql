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

#pragma once

#include <QDialog>

class QComboBox;
class QPushButton;

#include "core/events/events_info.h"

namespace fasto {
namespace qt {
namespace gui {
class GlassWidget;
class GraphWidget;
}
}
}

namespace fastonosql {

class ServerHistoryDialog
  : public QDialog {
  Q_OBJECT
 public:
  explicit ServerHistoryDialog(IServerSPtr server, QWidget* parent = 0);

 Q_SIGNALS:
  void showed();

 private Q_SLOTS:
  void startLoadServerHistoryInfo(const EventsInfo::ServerInfoHistoryRequest& req);
  void finishLoadServerHistoryInfo(const EventsInfo::ServerInfoHistoryResponce& res);
  void startClearServerHistory(const EventsInfo::ClearServerHistoryRequest& req);
  void finishClearServerHistory(const EventsInfo::ClearServerHistoryResponce& res);
  void snapShotAdd(ServerInfoSnapShoot snapshot);
  void clearHistory();

  void refreshInfoFields(int index);
  void refreshGraph(int index);

 protected:
  virtual void changeEvent(QEvent* e);
  virtual void showEvent(QShowEvent* e);

 private:
  void reset();
  void retranslateUi();
  void requestHistoryInfo();

  QWidget* settingsGraph_;
  QPushButton* clearHistory_;
  QComboBox* serverInfoGroupsNames_;
  QComboBox* serverInfoFields_;

  fasto::qt::gui::GraphWidget* graphWidget_;

  fasto::qt::gui::GlassWidget* glassWidget_;
  EventsInfo::ServerInfoHistoryResponce::infos_container_type infos_;
  const IServerSPtr server_;
};

}  // namespace fastonosql
