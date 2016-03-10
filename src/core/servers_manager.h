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

#include <vector>

#include "common/patterns/singleton_pattern.h"

#include "core/iserver.h"
#include "core/connection_settings.h"

namespace fastonosql {

class ServersManager
  : public QObject, public common::patterns::LazySingleton<ServersManager> {
  friend class common::patterns::LazySingleton<ServersManager>;
  Q_OBJECT
 public:
  typedef IServerSPtr server_t;
  typedef IClusterSPtr cluster_t;
  typedef std::vector<server_t> servers_container_t;

  server_t createServer(IConnectionSettingsBaseSPtr settings);
  cluster_t createCluster(IClusterSettingsBaseSPtr settings);

  common::Error testConnection(IConnectionSettingsBaseSPtr connection);
  common::Error discoveryConnection(IConnectionSettingsBaseSPtr connection,
                                    std::vector<ServerDiscoveryInfoSPtr>* inf);

  void clear();

 public Q_SLOTS:
  void closeServer(server_t server);
  void closeCluster(cluster_t cluster);

 private:
  ServersManager();
  ~ServersManager();

  servers_container_t servers_;
};

}  // namespace fastonosql
