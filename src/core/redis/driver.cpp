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

#include "core/redis/driver.h"

#include <stddef.h>                     // for size_t
#include <stdint.h>                     // for uint32_t

#include <memory>                       // for __shared_ptr, shared_ptr
#include <vector>                       // for vector

#include "common/convert2string.h"      // for ConvertFromString, etc
#include "common/file_system.h"         // for copy_file
#include "common/intrusive_ptr.h"       // for intrusive_ptr
#include "common/log_levels.h"          // for LEVEL_LOG::L_WARNING
#include "common/qt/utils_qt.h"         // for Event<>::value_type
#include "common/sprintf.h"             // for MemSPrintf
#include "common/value.h"               // for Value, ErrorValue, etc

#include "core/command.h"           // for createCommand, etc
#include "core/connection_types.h"
#include "core/db_key.h"                // for NDbKValue, NValue, ttl_t, etc
#include "core/events/events_info.h"
#include "core/server_property_info.h"  // for makeServerProperty, etc
#include "core/ssh_info.h"              // for SSHInfo

#include "core/redis/command.h"         // for Command
#include "core/redis/config.h"          // for Config
#include "core/redis/connection_settings.h"  // for ConnectionSettings
#include "core/redis/database.h"        // for DataBaseInfo
#include "core/redis/server_info.h"     // for ServerInfo, etc

#include "global/global.h"              // for FastoObjectCommandIPtr, etc

#define REDIS_SHUTDOWN "SHUTDOWN"
#define REDIS_BACKUP "SAVE"
#define REDIS_SET_PASSWORD_1ARGS_S "CONFIG SET requirepass %s"
#define REDIS_SET_MAX_CONNECTIONS_1ARGS_I "CONFIG SET maxclients %d"
#define REDIS_GET_DATABASES "CONFIG GET databases"
#define REDIS_GET_PROPERTY_SERVER "CONFIG GET *"

#define REDIS_GET_KEY_PATTERN_1ARGS_S "GET %s"
#define REDIS_GET_KEY_LIST_PATTERN_1ARGS_S "LRANGE %s 0 -1"
#define REDIS_GET_KEY_SET_PATTERN_1ARGS_S "SMEMBERS %s"
#define REDIS_GET_KEY_ZSET_PATTERN_1ARGS_S "ZRANGE %s 0 -1"
#define REDIS_GET_KEY_HASHM_PATTERN_1ARGS_S "HGETALL %s"

#define REDIS_SET_KEY_PATTERN_2ARGS_SS "SET %s %s"
#define REDIS_SET_KEY_LIST_PATTERN_2ARGS_SS "LPUSH %s %s"
#define REDIS_SET_KEY_SET_PATTERN_2ARGS_SS "SADD %s %s"
#define REDIS_SET_KEY_ZSET_PATTERN_2ARGS_SS "ZADD %s %s"
#define REDIS_SET_KEY_HASHM_PATTERN_2ARGS_SS "HMSET %s %s"

#define REDIS_GET_KEYS_PATTERN_3ARGS_ISI "SCAN %d MATCH %s COUNT %d"

#define REDIS_SET_DEFAULT_DATABASE_PATTERN_1ARGS_S "SELECT %s"
#define REDIS_FLUSHDB "FLUSHDB"

#define REDIS_CHANGE_TTL_2ARGS_SI "EXPIRE %s %d"
#define REDIS_PERSIST_KEY_1ARGS_S "PERSIST %s"
#define REDIS_DELETE_KEY_PATTERN_1ARGS_S "DEL %s"

namespace {

common::Value::Type convertFromStringRType(const std::string& type) {
  if (type.empty()) {
     return common::Value::TYPE_NULL;
  }

  if (type == "string") {
    return common::Value::TYPE_STRING;
  }  else if (type == "list") {
    return common::Value::TYPE_ARRAY;
  } else if (type == "set") {
    return common::Value::TYPE_SET;
  } else if (type == "hash") {
    return common::Value::TYPE_HASH;
  } else if (type == "zset") {
    return common::Value::TYPE_ZSET;
  } else {
    return common::Value::TYPE_NULL;
  }
}

}  // namespace

namespace fastonosql {
namespace core {
namespace redis {

namespace {

Command* createCommandFast(const std::string& input, common::Value::CommandLoggingType ct) {
  common::CommandValue* cmd = common::Value::createCommand(input, ct);
  Command* fs = new Command(nullptr, cmd, std::string());
  return fs;
}

}  // namespace

Driver::Driver(IConnectionSettingsBaseSPtr settings)
  : IDriverRemote(settings), impl_(new DBConnection(this)) {
  COMPILE_ASSERT(DBConnection::connection_t == REDIS, "DBConnection must be the same type as Driver!");
  CHECK(type() == REDIS);
}

Driver::~Driver() {
  delete impl_;
}

common::net::HostAndPort Driver::host() const {
  Config conf = impl_->config();
  return conf.host;
}

std::string Driver::nsSeparator() const {
  return impl_->nsSeparator();
}

std::string Driver::delimiter() const {
  return impl_->delimiter();
}

bool Driver::isInterrupted() const {
  return impl_->isInterrupted();
}

void Driver::setInterrupted(bool interrupted) {
  return impl_->setInterrupted(interrupted);
}

bool Driver::isConnected() const {
  return impl_->isConnected();
}

bool Driver::isAuthenticated() const {
  return impl_->isAuthenticated();
}

void Driver::currentDataBaseChanged(IDataBaseInfo* info) {
  setCurrentDatabaseInfo(info->clone());
}

void Driver::initImpl() {
}

void Driver::clearImpl() {
}

common::Error Driver::syncConnect() {
  ConnectionSettings* set = dynamic_cast<ConnectionSettings*>(settings_.get());  // +
  CHECK(set);
  RConfig rconf(set->info(), set->sshInfo());
  return impl_->connect(rconf);
}

common::Error Driver::syncDisconnect() {
  return impl_->disconnect();
}

common::Error Driver::executeImpl(int argc, char** argv, FastoObject* out) {
  return impl_->execute(argc, argv, out);
}

common::Error Driver::serverInfo(IServerInfo** info) {
  FastoObjectIPtr root = FastoObject::createRoot(INFO_REQUEST);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, INFO_REQUEST,
                                                        common::Value::C_INNER);
  common::Error res = execute(cmd);
  if (!res) {
    auto ch = root->childrens();
    if (ch.size()) {
      std::string content = common::ConvertToString(ch[0]);
      *info = makeRedisServerInfo(content);
    }

    if (*info == nullptr) {
      res = common::make_error_value("Invalid " INFO_REQUEST " command output",
                                     common::ErrorValue::E_ERROR);
    }
  }

  return res;
}

common::Error Driver::currentDataBaseInfo(IDataBaseInfo** info) {
  Config conf = impl_->config();
  return impl_->select(conf.dbnum, info);
}

void Driver::handleProcessCommandLineArgs(events::ProcessConfigArgsRequestEvent* ev) {
  const Config conf = impl_->config();
  /* Latency mode */
  if (conf.latency_mode) {
    latencyMode(ev);
  }

  /* Slave mode */
  if (conf.slave_mode) {
    slaveMode(ev);
  }

  /* Get RDB mode. */
  if (conf.getrdb_mode) {
    getRDBMode(ev);
  }

  /* Find big keys */
  if (conf.bigkeys) {
    findBigKeysMode(ev);
  }

  /* Stat mode */
  if (conf.stat_mode) {
    statMode(ev);
  }

  /* Scan mode */
  if (conf.scan_mode) {
    scanMode(ev);
  }

  interacteveMode(ev);

  QObject* sender = ev->sender();
  events::ProcessConfigArgsResponceEvent::value_type res(ev->value());
  reply(sender, new events::ProcessConfigArgsResponceEvent(this, res));
}

void Driver::handleShutdownEvent(events::ShutDownRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ShutDownResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_SHUTDOWN);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, REDIS_SHUTDOWN, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ShutDownResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleBackupEvent(events::BackupRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::BackupResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_BACKUP);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, REDIS_BACKUP, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    common::Error err = common::file_system::copy_file("/var/lib/redis/dump.rdb", res.path);
    if (err && err->isError()) {
      res.setErrorInfo(err);
    }
  }
  notifyProgress(sender, 75);
  reply(sender, new events::BackupResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleExportEvent(events::ExportRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ExportResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  common::Error err = common::file_system::copy_file(res.path, "/var/lib/redis/dump.rdb");
  if (err && err->isError()) {
    res.setErrorInfo(err);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ExportResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleChangePasswordEvent(events::ChangePasswordRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangePasswordResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  std::string patternResult = common::MemSPrintf(REDIS_SET_PASSWORD_1ARGS_S, res.new_password);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, patternResult, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  notifyProgress(sender, 75);
  reply(sender, new events::ChangePasswordResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleChangeMaxConnectionEvent(events::ChangeMaxConnectionRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangeMaxConnectionResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  std::string patternResult = common::MemSPrintf(REDIS_SET_MAX_CONNECTIONS_1ARGS_I, res.max_connection);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, patternResult, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  notifyProgress(sender, 75);
  reply(sender, new events::ChangeMaxConnectionResponceEvent(this, res));
  notifyProgress(sender, 100);
}

common::Error Driver::interacteveMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type res(this, InteractiveMode);
  reply(sender, new events::EnterModeEvent(this, res));

  events::LeaveModeEvent::value_type res2(this, InteractiveMode);
  reply(sender, new events::LeaveModeEvent(this, res2));
  notifyProgress(sender, 100);
  return common::Error();
}

common::Error Driver::latencyMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, LatencyMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, LatencyMode);
  RootLocker lock = make_locker(sender, LATENCY_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->latencyMode(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

common::Error Driver::slaveMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, SlaveMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, SlaveMode);
  RootLocker lock = make_locker(sender, SYNC_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->slaveMode(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

common::Error Driver::getRDBMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, GetRDBMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, GetRDBMode);
  RootLocker lock = make_locker(sender, RDM_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->getRDB(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

common::Error Driver::findBigKeysMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, FindBigKeysMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, FindBigKeysMode);
  RootLocker lock = make_locker(sender, FIND_BIG_KEYS_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->findBigKeys(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

common::Error Driver::statMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, StatMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, StatMode);
  RootLocker lock = make_locker(sender, STAT_MODE_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->statMode(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

common::Error Driver::scanMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type resEv(this, ScanMode);
  reply(sender, new events::EnterModeEvent(this, resEv));

  events::LeaveModeEvent::value_type res(this, ScanMode);
  RootLocker lock = make_locker(sender, SCAN_MODE_REQUEST);

  FastoObjectIPtr obj = lock.root();
  common::Error er = impl_->scanMode(obj.get());
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::LeaveModeEvent(this, res));
  notifyProgress(sender, 100);
  return er;
}

void Driver::handleExecuteEvent(events::ExecuteRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ExecuteResponceEvent::value_type res(ev->value());
  const std::string inputLine = res.text;
  if (inputLine.empty()) {
    res.setErrorInfo(common::make_error_value("Empty command line.", common::ErrorValue::E_ERROR));
    reply(sender, new events::ExecuteResponceEvent(this, res));
    notifyProgress(sender, 100);
  }

  size_t length = inputLine.length();
  int offset = 0;
  RootLocker lock = make_locker(sender, inputLine);
  FastoObjectIPtr obj = lock.root();
  const double step = 100.0 / length;
  for (size_t i = 0; i < length; ++i) {
    if (isInterrupted()) {
      res.setErrorInfo(common::make_error_value("Interrupted exec.",
                                                common::ErrorValue::E_INTERRUPTED,
                                                common::logging::L_WARNING));
      break;
    }

    if (inputLine[i] == '\n' || i == length - 1) {
      notifyProgress(sender, step * i);
      std::string command;
      if (i == length - 1) {
        command = inputLine.substr(offset);
      } else {
        command = inputLine.substr(offset, i - offset);
      }

      offset = i + 1;
      FastoObjectCommand* cmd = CreateCommand<Command>(obj, command, common::Value::C_USER);
      common::Error er = execute(cmd);
      if (er && er->isError()) {
        res.setErrorInfo(er);
        break;
      }
    }
  }

  reply(sender, new events::ExecuteResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleCommandRequestEvent(events::CommandRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::CommandResponceEvent::value_type res(ev->value());
  std::string cmdtext;
  common::Error er = commandByType(res.cmd, &cmdtext);
  if (er && er->isError()) {
    res.setErrorInfo(er);
    reply(sender, new events::CommandResponceEvent(this, res));
    notifyProgress(sender, 100);
    return;
  }

  RootLocker lock = make_locker(sender, cmdtext);
  FastoObjectIPtr obj = lock.root();
  FastoObjectCommand* cmd = CreateCommand<Command>(obj, cmdtext, common::Value::C_INNER);
  notifyProgress(sender, 50);
  er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  reply(sender, new events::CommandResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::LoadDatabasesInfoResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_GET_DATABASES);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, REDIS_GET_DATABASES,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    FastoObject::childs_t rchildrens = cmd->childrens();
    if (rchildrens.size()) {
      CHECK_EQ(rchildrens.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(rchildrens[0]);  // +
      if (!array) {
        goto done;
      }
      common::ArrayValue* ar = array->array();
      if (!ar) {
        goto done;
      }

      IDataBaseInfoSPtr curdb = currentDatabaseInfo();
      CHECK(curdb);
      if (ar->size() == 2) {
        std::string scountDb;
        bool isok = ar->getString(1, &scountDb);
        if (isok) {
            size_t countDb = common::ConvertFromString<size_t>(scountDb);
            if (countDb > 0) {
              for (size_t i = 0; i < countDb; ++i) {
                IDataBaseInfoSPtr dbInf(new DataBaseInfo(common::ConvertToString(i), false, 0));
                if (dbInf->name() == curdb->name()) {
                  res.databases.push_back(curdb);
                } else {
                  res.databases.push_back(dbInf);
                }
              }
            }
        }
      } else {
        res.databases.push_back(curdb);
      }
    }
  }
done:
  notifyProgress(sender, 75);
  reply(sender, new events::LoadDatabasesInfoResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleLoadDatabaseContentEvent(events::LoadDatabaseContentRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::LoadDatabaseContentResponceEvent::value_type res(ev->value());
  std::string patternResult = common::MemSPrintf(REDIS_GET_KEYS_PATTERN_3ARGS_ISI, res.cursor_in,
                                                 res.pattern, res.count_keys);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, patternResult, common::Value::C_INNER);
  common::Error err = execute(cmd);
  if (err && err->isError()) {
    res.setErrorInfo(err);
  } else {
    FastoObject::childs_t rchildrens = cmd->childrens();
    if (rchildrens.size()) {
      CHECK_EQ(rchildrens.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(rchildrens[0]);  // +
      if (!array) {
        goto done;
      }

      common::ArrayValue* arm = array->array();
      if (!arm->size()) {
        goto done;
      }

      std::string cursor;
      bool isok = arm->getString(0, &cursor);
      if (!isok) {
        goto done;
      }

      res.cursor_out = common::ConvertFromString<uint32_t>(cursor);

      rchildrens = array->childrens();
      if (!rchildrens.size()) {
        goto done;
      }

      FastoObject* obj = rchildrens[0];
      FastoObjectArray* arr = dynamic_cast<FastoObjectArray*>(obj);  // +
      if (!arr) {
        goto done;
      }

      common::ArrayValue* ar = arr->array();
      std::vector<FastoObjectCommandIPtr> cmds;
      cmds.reserve(ar->size() * 2);
      for (size_t i = 0; i < ar->size(); ++i) {
        std::string key;
        bool isok = ar->getString(i, &key);
        if (isok) {
          NKey k(key);
          NDbKValue dbv(k, NValue());
          cmds.push_back(createCommandFast("TYPE " + key, common::Value::C_INNER));
          cmds.push_back(createCommandFast("TTL " + key, common::Value::C_INNER));
          res.keys.push_back(dbv);
        }
      }

      err = impl_->executeAsPipeline(cmds);
      if (err && err->isError()) {
        goto done;
      }

      for (size_t i = 0; i < res.keys.size(); ++i) {
        FastoObjectIPtr cmdType = cmds[i*2];
        FastoObject::childs_t tchildrens = cmdType->childrens();
        if (tchildrens.size()) {
          DCHECK_EQ(tchildrens.size(), 1);
          if (tchildrens.size() == 1) {
            FastoObject* type = tchildrens[0];
            std::string typeRedis = type->toString();
            common::Value::Type ctype = convertFromStringRType(typeRedis);
            common::Value* emptyval = common::Value::createEmptyValueFromType(ctype);
            common::ValueSPtr val = make_value(emptyval);
            res.keys[i].setValue(val);
          }
        }

        FastoObjectIPtr cmdType2 = cmds[i * 2 + 1];
        tchildrens = cmdType2->childrens();
        if (tchildrens.size()) {
          DCHECK_EQ(tchildrens.size(), 1);
          if (tchildrens.size() == 1) {
            FastoObject* fttl = tchildrens[0];
            auto vttl = fttl->value();
            ttl_t ttl = 0;
            if (vttl->getAsInteger(&ttl)) {
                res.keys[i].setTTL(ttl);
            }
          }
        }
      }

      err = impl_->dbkcount(&res.db_keys_count);
      MCHECK(!err);
    }
  }
done:
  notifyProgress(sender, 75);
  reply(sender, new events::LoadDatabaseContentResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleClearDatabaseEvent(events::ClearDatabaseRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ClearDatabaseResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_FLUSHDB);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, REDIS_FLUSHDB,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ClearDatabaseResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::SetDefaultDatabaseResponceEvent::value_type res(ev->value());
  std::string setDefCommand = common::MemSPrintf(REDIS_SET_DEFAULT_DATABASE_PATTERN_1ARGS_S,
                                                 res.inf->name());
  FastoObjectIPtr root = FastoObject::createRoot(setDefCommand);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, setDefCommand,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::SetDefaultDatabaseResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleLoadServerPropertyEvent(events::ServerPropertyInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ServerPropertyInfoResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_GET_PROPERTY_SERVER);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, REDIS_GET_PROPERTY_SERVER,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    FastoObject::childs_t ch = cmd->childrens();
    if (ch.size()) {
      CHECK_EQ(ch.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(ch[0]);  // +
      if (array) {
        res.info = makeServerProperty(array);
      }
    }
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ServerPropertyInfoResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void Driver::handleServerPropertyChangeEvent(events::ChangeServerPropertyInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangeServerPropertyInfoResponceEvent::value_type res(ev->value());

  notifyProgress(sender, 50);
  std::string changeRequest = "CONFIG SET " + res.new_item.first + " " + res.new_item.second;
  FastoObjectIPtr root = FastoObject::createRoot(changeRequest);
  FastoObjectCommand* cmd = CreateCommand<Command>(root, changeRequest,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    res.is_change = true;
  }
  notifyProgress(sender, 75);
      reply(sender, new events::ChangeServerPropertyInfoResponceEvent(this, res));
  notifyProgress(sender, 100);
}

common::Error Driver::commandDeleteImpl(CommandDeleteKey* command,
                                             std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  NDbKValue key = command->key();
  std::string key_str = key.keyString();
  *cmdstring = common::MemSPrintf(REDIS_DELETE_KEY_PATTERN_1ARGS_S, key_str);
  return common::Error();
}

common::Error Driver::commandLoadImpl(CommandLoadKey* command, std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  std::string key_str = key.keyString();
  if (key.type() == common::Value::TYPE_ARRAY) {
    patternResult = common::MemSPrintf(REDIS_GET_KEY_LIST_PATTERN_1ARGS_S, key_str);
  } else if (key.type() == common::Value::TYPE_SET) {
    patternResult = common::MemSPrintf(REDIS_GET_KEY_SET_PATTERN_1ARGS_S, key_str);
  } else if (key.type() == common::Value::TYPE_ZSET) {
    patternResult = common::MemSPrintf(REDIS_GET_KEY_ZSET_PATTERN_1ARGS_S, key_str);
  } else if (key.type() == common::Value::TYPE_HASH) {
    patternResult = common::MemSPrintf(REDIS_GET_KEY_HASHM_PATTERN_1ARGS_S, key_str);
  } else {
    patternResult = common::MemSPrintf(REDIS_GET_KEY_PATTERN_1ARGS_S, key_str);
  }

  *cmdstring = patternResult;
  return common::Error();
}

common::Error Driver::commandCreateImpl(CommandCreateKey* command,
                                             std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  NValue val = command->value();
  common::Value* rval = val.get();
  std::string key_str = key.keyString();
  std::string value_str = common::ConvertToString(rval, " ");
  common::Value::Type t = key.type();
  if (t == common::Value::TYPE_ARRAY) {
    patternResult = common::MemSPrintf(REDIS_SET_KEY_LIST_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_SET) {
    patternResult = common::MemSPrintf(REDIS_SET_KEY_SET_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_ZSET) {
    patternResult = common::MemSPrintf(REDIS_SET_KEY_ZSET_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_HASH) {
    patternResult = common::MemSPrintf(REDIS_SET_KEY_HASHM_PATTERN_2ARGS_SS, key_str, value_str);
  } else {
    patternResult = common::MemSPrintf(REDIS_SET_KEY_PATTERN_2ARGS_SS, key_str, value_str);
  }

  *cmdstring = patternResult;
  return common::Error();
}

common::Error Driver::commandChangeTTLImpl(CommandChangeTTL* command,
                                                std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  ttl_t new_ttl = command->newTTL();
  std::string key_str = key.keyString();
  if (new_ttl == -1) {
    patternResult = common::MemSPrintf(REDIS_PERSIST_KEY_1ARGS_S, key_str);
  } else {
    patternResult = common::MemSPrintf(REDIS_CHANGE_TTL_2ARGS_SI, key_str, new_ttl);
  }

  *cmdstring = patternResult;
  return common::Error();
}

IServerInfoSPtr Driver::makeServerInfoFromString(const std::string& val) {
  IServerInfoSPtr res(makeRedisServerInfo(val));
  return res;
}

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
