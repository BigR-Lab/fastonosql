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

#include "core/idriver.h"

#include "core/ssdb/ssdb_raw.h"

namespace fastonosql {
namespace ssdb {

common::Error testConnection(SsdbConnectionSettings* settings);

class SsdbDriver
  : public IDriver {
  Q_OBJECT
 public:
  explicit SsdbDriver(IConnectionSettingsBaseSPtr settings);
  virtual ~SsdbDriver();

  virtual bool isConnected() const;
  virtual bool isAuthenticated() const;
  common::net::hostAndPort address() const;
  virtual std::string outputDelemitr() const;

 private:
  virtual void initImpl();
  virtual void clearImpl();

  virtual common::Error executeImpl(int argc, char **argv, FastoObject* out);
  virtual common::Error serverInfo(ServerInfo** info);
  virtual common::Error serverDiscoveryInfo(ServerDiscoveryInfo** dinfo, ServerInfo** sinfo,
                                            IDataBaseInfo** dbinfo);
  virtual common::Error currentDataBaseInfo(IDataBaseInfo** info);

  virtual void handleConnectEvent(events::ConnectRequestEvent* ev);
  virtual void handleDisconnectEvent(events::DisconnectRequestEvent* ev);
  virtual void handleExecuteEvent(events::ExecuteRequestEvent* ev);
  virtual void handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoRequestEvent* ev);
  virtual void handleLoadServerInfoEvent(events::ServerInfoRequestEvent* ev);
  virtual void handleProcessCommandLineArgs(events::ProcessConfigArgsRequestEvent* ev);

  // ============== commands =============//
  virtual common::Error commandDeleteImpl(CommandDeleteKey* command,
                                          std::string* cmdstring) const WARN_UNUSED_RESULT;
  virtual common::Error commandLoadImpl(CommandLoadKey* command,
                                        std::string* cmdstring) const WARN_UNUSED_RESULT;
  virtual common::Error commandCreateImpl(CommandCreateKey* command,
                                          std::string* cmdstring) const WARN_UNUSED_RESULT;
  virtual common::Error commandChangeTTLImpl(CommandChangeTTL* command,
                                             std::string* cmdstring) const WARN_UNUSED_RESULT;
  // ============== commands =============//

  // ============== database =============//
  virtual void handleLoadDatabaseContentEvent(events::LoadDatabaseContentRequestEvent* ev);
  virtual void handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseRequestEvent* ev);
  // ============== database =============//
  // ============== command =============//
  virtual void handleCommandRequestEvent(events::CommandRequestEvent* ev);
  // ============== command =============//
  ServerInfoSPtr makeServerInfoFromString(const std::string& val);

 private:
  SsdbRaw* const impl_;
};

}  // namespace ssdb
}  // namespace fastonosql
