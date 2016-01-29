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

#include "common/patterns/singleton_pattern.h"

#include "core/iserver.h"
#include "core/connection_settings.h"

namespace fastonosql {

class ServersManager
  : public QObject, public common::patterns::LazySingleton<ServersManager>
{
  friend class common::patterns::LazySingleton<ServersManager>;
  Q_OBJECT

public:
  typedef std::vector<IServerSPtr> ServersContainer;

  IServerSPtr createServer(IConnectionSettingsBaseSPtr settings);
  IClusterSPtr createCluster(IClusterSettingsBaseSPtr settings);

  common::Error testConnection(IConnectionSettingsBaseSPtr connection);
  common::Error discoveryConnection(IConnectionSettingsBaseSPtr connection, std::vector<ServerDiscoveryInfoSPtr>& inf);

  void setSyncServers(bool isSync);
  void clear();

public Q_SLOTS:
  void closeServer(IServerSPtr server);
  void closeCluster(IClusterSPtr cluster);

private:
  template<class Server, class Driver>
  IServer* make_server(IServerSPtr pser, IConnectionSettingsBaseSPtr settings);

  ServersManager();
  ~ServersManager();

  void refreshSyncServers();
  IServerSPtr findServerBySetting(const IConnectionSettingsBaseSPtr& settings) const;
  std::vector<QObject*> findAllListeners(const IDriverSPtr& drv) const;

  ServersContainer servers_;
  bool syncServers_;
};

}
