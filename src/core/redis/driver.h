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

#include <string>  // for string

#include <common/error.h>      // for Error
#include <common/macros.h>     // for WARN_UNUSED_RESULT
#include <common/net/types.h>  // for HostAndPort
#include <common/value.h>      // for Value, etc

#include "core/connection_settings/connection_settings.h"
#include "core/driver/idriver_remote.h"  // for IDriverRemote
#include "core/events/events.h"
#include "core/server/iserver_info.h"             // for IServerInfo (ptr only), etc
#include "core/translator/icommand_translator.h"  // for translator_t

#include "global/global.h"  // for FastoObject (ptr only), etc

namespace fastonosql {
namespace core {
class IDataBaseInfo;
}
}
namespace fastonosql {
namespace core {
namespace redis {
class DBConnection;
}
}
}

namespace fastonosql {
namespace core {
namespace redis {

class Driver : public IDriverRemote {
  Q_OBJECT
 public:
  explicit Driver(IConnectionSettingsBaseSPtr settings);
  virtual ~Driver();

  virtual bool isInterrupted() const override;
  virtual void setInterrupted(bool interrupted) override;

  virtual translator_t translator() const;

  virtual bool isConnected() const;
  virtual bool isAuthenticated() const;
  virtual common::net::HostAndPort host() const;
  virtual std::string nsSeparator() const;
  virtual std::string delimiter() const;

 private:
  virtual void initImpl();
  virtual void clearImpl();

  virtual FastoObjectCommandIPtr createCommand(FastoObject* parent,
                                               const std::string& input,
                                               common::Value::CommandLoggingType ct) override;

  virtual FastoObjectCommandIPtr createCommandFast(const std::string& input,
                                                   common::Value::CommandLoggingType ct) override;

  virtual common::Error syncConnect() override WARN_UNUSED_RESULT;
  virtual common::Error syncDisconnect() override WARN_UNUSED_RESULT;

  virtual common::Error executeImpl(int argc, const char** argv, FastoObject* out);

  virtual common::Error serverInfo(IServerInfo** info);
  virtual common::Error currentDataBaseInfo(IDataBaseInfo** info);

  virtual void handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoRequestEvent* ev);
  virtual void handleLoadServerPropertyEvent(events::ServerPropertyInfoRequestEvent* ev);
  virtual void handleServerPropertyChangeEvent(events::ChangeServerPropertyInfoRequestEvent* ev);
  virtual void handleProcessCommandLineArgs(events::ProcessConfigArgsRequestEvent* ev);
  virtual void handleShutdownEvent(events::ShutDownRequestEvent* ev);
  virtual void handleBackupEvent(events::BackupRequestEvent* ev);
  virtual void handleExportEvent(events::ExportRequestEvent* ev);
  virtual void handleChangePasswordEvent(events::ChangePasswordRequestEvent* ev);
  virtual void handleChangeMaxConnectionEvent(events::ChangeMaxConnectionRequestEvent* ev);

  virtual void handleLoadDatabaseContentEvent(events::LoadDatabaseContentRequestEvent* ev);
  virtual void handleClearDatabaseEvent(events::ClearDatabaseRequestEvent* ev);
  virtual void handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseRequestEvent* ev);

  IServerInfoSPtr makeServerInfoFromString(const std::string& val);

  DBConnection* const impl_;

  common::Error interacteveMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error latencyMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error slaveMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error getRDBMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error findBigKeysMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error statMode(events::ProcessConfigArgsRequestEvent* ev);
  common::Error scanMode(events::ProcessConfigArgsRequestEvent* ev);
};

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
