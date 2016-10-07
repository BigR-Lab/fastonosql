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

#include <memory>  // for enable_shared_from_this
#include <string>  // for string
#include <vector>  // for vector

#include <common/value.h>  // for ValueSPtr

#include "core/connection_types.h"                // for connectionTypes
#include "core/core_fwd.h"                        // for IDatabaseSPtr
#include "core/database/idatabase_info.h"         // for IDataBaseInfoSPtr
#include "core/db_key.h"                          // for NKey (ptr only), etc
#include "core/events/events.h"                   // for BackupResponceEvent, etc
#include "core/server/iserver_base.h"             // for IServerBase
#include "core/server/iserver_info.h"             // for IServerInfoSPtr, etc
#include "core/translator/icommand_translator.h"  // for translator_t

#include "global/global.h"  // for FastoObject (ptr only), etc

namespace fastonosql {
namespace core {

class IDriver;

class IServer : public IServerBase, public std::enable_shared_from_this<IServer> {
  Q_OBJECT
 public:
  typedef IDataBaseInfoSPtr database_t;
  typedef std::vector<database_t> databases_t;
  virtual ~IServer();

  // sync methods
  void stopCurrentEvent();
  bool isConnected() const;
  bool isAuthenticated() const;
  bool isCanRemote() const;

  translator_t translator() const;

  connectionTypes type() const;
  virtual std::string name() const;

  IDataBaseInfoSPtr currentDatabaseInfo() const;
  IServerInfoSPtr serverInfo() const;

  std::string delimiter() const;
  std::string nsSeparator() const;
  IDatabaseSPtr createDatabaseByInfo(IDataBaseInfoSPtr inf);
  bool containsDatabase(IDataBaseInfoSPtr inf) const;

 Q_SIGNALS:  // only direct connections
  void startedConnect(const events_info::ConnectInfoRequest& req);
  void finishedConnect(const events_info::ConnectInfoResponce& res);

  void startedDisconnect(const events_info::DisConnectInfoRequest& req);
  void finishedDisconnect(const events_info::DisConnectInfoResponce& res);

  void startedShutdown(const events_info::ShutDownInfoRequest& req);
  void finishedShutdown(const events_info::ShutDownInfoResponce& res);

  void startedBackup(const events_info::BackupInfoRequest& req);
  void finishedBackup(const events_info::BackupInfoResponce& res);

  void startedExport(const events_info::ExportInfoRequest& req);
  void finishedExport(const events_info::ExportInfoResponce& res);

  void startedChangePassword(const events_info::ChangePasswordRequest& req);
  void finishedChangePassword(const events_info::ChangePasswordResponce& res);

  void startedChangeMaxConnection(const events_info::ChangeMaxConnectionRequest& req);
  void finishedChangeMaxConnection(const events_info::ChangeMaxConnectionResponce& res);

  void startedExecute(const events_info::ExecuteInfoRequest& req);
  void finishedExecute(const events_info::ExecuteInfoResponce& res);

  void startedLoadDatabases(const events_info::LoadDatabasesInfoRequest& req);
  void finishedLoadDatabases(const events_info::LoadDatabasesInfoResponce& res);

  void startedLoadServerInfo(const events_info::ServerInfoRequest& req);
  void finishedLoadServerInfo(const events_info::ServerInfoResponce& res);

  void startedLoadServerHistoryInfo(const events_info::ServerInfoHistoryRequest& req);
  void finishedLoadServerHistoryInfo(const events_info::ServerInfoHistoryResponce& res);

  void startedClearServerHistory(const events_info::ClearServerHistoryRequest& req);
  void finishedClearServerHistory(const events_info::ClearServerHistoryResponce& req);

  void startedLoadServerProperty(const events_info::ServerPropertyInfoRequest& req);
  void finishedLoadServerProperty(const events_info::ServerPropertyInfoResponce& res);

  void startedChangeServerProperty(const events_info::ChangeServerPropertyInfoRequest& req);
  void finishedChangeServerProperty(const events_info::ChangeServerPropertyInfoResponce& res);

  void progressChanged(const events_info::ProgressInfoResponce& res);

  void enteredMode(const events_info::EnterModeInfo& res);
  void leavedMode(const events_info::LeaveModeInfo& res);

  void rootCreated(const events_info::CommandRootCreatedInfo& res);
  void rootCompleated(const events_info::CommandRootCompleatedInfo& res);

  void startedLoadDataBaseContent(const events_info::LoadDatabaseContentRequest& req);
  void finishedLoadDatabaseContent(const events_info::LoadDatabaseContentResponce& res);

  void startedSetDefaultDatabase(const events_info::SetDefaultDatabaseRequest& req);
  void finishedSetDefaultDatabase(const events_info::SetDefaultDatabaseResponce& res);

  void startedClearDatabase(const events_info::ClearDatabaseRequest& req);
  void finishedClearDatabase(const events_info::ClearDatabaseResponce& res);

  void startedLoadDiscoveryInfo(const events_info::DiscoveryInfoRequest& res);
  void finishedLoadDiscoveryInfo(const events_info::DiscoveryInfoResponce& res);

 Q_SIGNALS:
  void addedChild(FastoObjectIPtr child);
  void itemUpdated(FastoObject* item, common::ValueSPtr val);
  void serverInfoSnapShoot(core::ServerInfoSnapShoot shot);

  void keyRemoved(core::IDataBaseInfoSPtr db, core::NKey key);
  void keyAdded(core::IDataBaseInfoSPtr db, core::NDbKValue key);
  void keyLoaded(core::IDataBaseInfoSPtr db, core::NDbKValue key);
  void keyRenamed(core::IDataBaseInfoSPtr db, core::NKey key, std::string new_name);
  void keyTTLChanged(core::IDataBaseInfoSPtr db, core::NKey key, core::ttl_t ttl);

 public:
  // async methods
  void connect(
      const events_info::ConnectInfoRequest& req);  // signals: startedConnect, finishedConnect
  void disconnect(const events_info::DisConnectInfoRequest& req);  // signals: startedDisconnect,
                                                                   // finishedDisconnect
  void loadDatabases(
      const events_info::LoadDatabasesInfoRequest& req);  // signals: startedLoadDatabases,
                                                          // finishedLoadDatabases
  void loadDatabaseContent(
      const events_info::LoadDatabaseContentRequest& req);  // signals: startedLoadDataBaseContent,
                                                            // finishedLoadDatabaseContent
  void setDefaultDB(
      const events_info::SetDefaultDatabaseRequest& req);  // signals: startedSetDefaultDatabase,
                                                           // finishedSetDefaultDatabase
  void clearDB(const events_info::ClearDatabaseRequest& req);  // signals: startedClearDatabase,
                                                               // finishedClearDatabase
  void execute(const events_info::ExecuteInfoRequest& req);    // signals: startedExecute

  void shutDown(const events_info::ShutDownInfoRequest& req);  // signals: startedShutdown,
                                                               // finishedShutdown
  void backupToPath(
      const events_info::BackupInfoRequest& req);  // signals: startedBackup, finishedBackup
  void exportFromPath(
      const events_info::ExportInfoRequest& req);  // signals: startedExport, finishedExport
  void changePassword(
      const events_info::ChangePasswordRequest& req);  // signals: startedChangePassword,
                                                       // finishedChangePassword
  void setMaxConnection(
      const events_info::ChangeMaxConnectionRequest& req);  // signals: startedChangeMaxConnection,
                                                            // finishedChangeMaxConnection
  void loadServerInfo(const events_info::ServerInfoRequest& req);  // signals:
  // startedLoadServerInfo,
  // finishedLoadServerInfo
  void serverProperty(
      const events_info::ServerPropertyInfoRequest& req);  // signals: startedLoadServerProperty,
                                                           // finishedLoadServerProperty
  void requestHistoryInfo(
      const events_info::ServerInfoHistoryRequest& req);  // signals: startedLoadServerHistoryInfo,
                                                          // finishedLoadServerHistoryInfo
  void clearHistory(
      const events_info::ClearServerHistoryRequest& req);  // signals: startedClearServerHistory,
                                                           // finishedClearServerHistory
  void changeProperty(const events_info::ChangeServerPropertyInfoRequest&
                          req);  // signals: startedChangeServerProperty,
                                 // finishedChangeServerProperty

 protected:
  explicit IServer(IDriver* drv);  // take ownerships

  virtual void customEvent(QEvent* event);

  virtual IDatabaseSPtr createDatabase(IDataBaseInfoSPtr info) = 0;
  void notify(QEvent* ev);

  // handle server events
  virtual void handleConnectEvent(events::ConnectResponceEvent* ev);
  virtual void handleDisconnectEvent(events::DisconnectResponceEvent* ev);
  virtual void handleLoadServerInfoEvent(events::ServerInfoResponceEvent* ev);
  virtual void handleLoadServerPropertyEvent(events::ServerPropertyInfoResponceEvent* ev);
  virtual void handleServerPropertyChangeEvent(events::ChangeServerPropertyInfoResponceEvent* ev);
  virtual void handleShutdownEvent(events::ShutDownResponceEvent* ev);
  virtual void handleBackupEvent(events::BackupResponceEvent* ev);
  virtual void handleExportEvent(events::ExportResponceEvent* ev);
  virtual void handleChangePasswordEvent(events::ChangePasswordResponceEvent* ev);
  virtual void handleChangeMaxConnection(events::ChangeMaxConnectionResponceEvent* ev);
  virtual void handleExecuteResponceEvent(events::ExecuteResponceEvent* ev);

  // handle database events
  virtual void handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoResponceEvent* ev);
  virtual void handleLoadDatabaseContentEvent(events::LoadDatabaseContentResponceEvent* ev);
  virtual void handleClearDatabaseResponceEvent(events::ClearDatabaseResponceEvent* ev);
  virtual void handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseResponceEvent* ev);

  // handle command events
  virtual void handleDiscoveryInfoResponceEvent(events::DiscoveryInfoResponceEvent* ev);

  IDriver* const drv_;
  databases_t databases_;

 private:
  void handleEnterModeEvent(events::EnterModeEvent* ev);
  void handleLeaveModeEvent(events::LeaveModeEvent* ev);

  // handle info events
  void handleLoadServerInfoHistoryEvent(events::ServerInfoHistoryResponceEvent* ev);
  void handleClearServerHistoryResponceEvent(events::ClearServerHistoryResponceEvent* ev);

  void processConfigArgs(const events_info::ProcessConfigArgsInfoRequest& req);
  void processDiscoveryInfo(const events_info::DiscoveryInfoRequest& req);
};

}  // namespace core
}  // namespace fastonosql
