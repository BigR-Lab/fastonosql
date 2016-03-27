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

#pragma once

#include <QTreeWidgetItem>

#include "core/connection_settings.h"

namespace fastonosql {
namespace gui {

class ConnectionListWidgetItem
  : public QTreeWidgetItem {
 public:
  explicit ConnectionListWidgetItem(core::IConnectionSettingsBaseSPtr connection);
  void setConnection(core::IConnectionSettingsBaseSPtr cons);
  core::IConnectionSettingsBaseSPtr connection() const;

 private:
  core::IConnectionSettingsBaseSPtr connection_;
};

class ConnectionListWidgetItemEx
  : public ConnectionListWidgetItem {
 public:
  ConnectionListWidgetItemEx(core::IConnectionSettingsBaseSPtr connection, core::serverTypes st);
};

class ClusterConnectionListWidgetItem
      : public QTreeWidgetItem {
 public:
  explicit ClusterConnectionListWidgetItem(core::IClusterSettingsBaseSPtr connection);
  void setConnection(core::IClusterSettingsBaseSPtr cons);
  core::IClusterSettingsBaseSPtr connection() const;

 private:
  core::IClusterSettingsBaseSPtr connection_;
};

}  // namespace gui
}  // namespace fastonosql
