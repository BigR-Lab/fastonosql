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

#include "core/redis/redis_driver.h"

#include "common/file_system.h"
#include "common/utils.h"

#include "core/command_logger.h"

#define SHUTDOWN "SHUTDOWN"
#define BACKUP "SAVE"
#define SET_PASSWORD_1ARGS_S "CONFIG SET requirepass %s"
#define SET_MAX_CONNECTIONS_1ARGS_I "CONFIG SET maxclients %d"
#define GET_DATABASES "CONFIG GET databases"
#define GET_PROPERTY_SERVER "CONFIG GET *"

#define GET_KEY_PATTERN_1ARGS_S "GET %s"
#define GET_KEY_LIST_PATTERN_1ARGS_S "LRANGE %s 0 -1"
#define GET_KEY_SET_PATTERN_1ARGS_S "SMEMBERS %s"
#define GET_KEY_ZSET_PATTERN_1ARGS_S "ZRANGE %s 0 -1"
#define GET_KEY_HASH_PATTERN_1ARGS_S "HGETALL %s"

#define SET_KEY_PATTERN_2ARGS_SS "SET %s %s"
#define SET_KEY_LIST_PATTERN_2ARGS_SS "LPUSH %s %s"
#define SET_KEY_SET_PATTERN_2ARGS_SS "SADD %s %s"
#define SET_KEY_ZSET_PATTERN_2ARGS_SS "ZADD %s %s"
#define SET_KEY_HASH_PATTERN_2ARGS_SS "HMSET %s %s"

#define GET_KEYS_PATTERN_3ARGS_ISI "SCAN %d MATCH %s COUNT %d"

#define SET_DEFAULT_DATABASE_PATTERN_1ARGS_S "SELECT %s"
#define REDIS_FLUSHDB "FLUSHDB"

#define CHANGE_TTL_2ARGS_SI "EXPIRE %s %d"
#define PERSIST_KEY_1ARGS_S "PERSIST %s"
#define DELETE_KEY_PATTERN_1ARGS_S "DEL %s"

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

}

namespace fastonosql {
namespace core {
namespace redis {

namespace {

RedisCommand* createCommandFast(const std::string& input, common::Value::CommandLoggingType ct) {
  common::CommandValue* cmd = common::Value::createCommand(input, ct);
  RedisCommand* fs = new RedisCommand(nullptr, cmd, std::string());
  return fs;
}

}

RedisDriver::RedisDriver(IConnectionSettingsBaseSPtr settings)
  : IDriverRemote(settings), impl_(new RedisRaw(this)) {
  CHECK(type() == REDIS);
}

RedisDriver::~RedisDriver() {
  delete impl_;
}

common::net::hostAndPort RedisDriver::host() const {
  return impl_->config_.host;
}

std::string RedisDriver::outputDelemitr() const {
  return impl_->config_.delimiter;
}

bool RedisDriver::isInterrupted() const {
  return IDriverRemote::isInterrupted();
}

bool RedisDriver::isConnected() const {
  return impl_->isConnected();
}

bool RedisDriver::isAuthenticated() const {
  return impl_->isAuthenticated();
}

void RedisDriver::currentDataBaseChanged(IDataBaseInfo* info) {
  setCurrentDatabaseInfo(info->clone());
}

void RedisDriver::initImpl() {
}

void RedisDriver::clearImpl() {
}

common::Error RedisDriver::executeImpl(int argc, char** argv, FastoObject* out) {
  return impl_->execute(argc, argv, out);
}

common::Error RedisDriver::serverInfo(IServerInfo** info) {
  FastoObjectIPtr root = FastoObject::createRoot(INFO_REQUEST);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, INFO_REQUEST,
                                                        common::Value::C_INNER);
  common::Error res = execute(cmd);
  if (!res) {
    auto ch = root->childrens();
    if (ch.size()) {
      *info = makeRedisServerInfo(ch[0]);
    }

    if (*info == nullptr) {
      res = common::make_error_value("Invalid " INFO_REQUEST " command output",
                                     common::ErrorValue::E_ERROR);
    }
  }

  return res;
}

common::Error RedisDriver::serverDiscoveryInfo(ServerDiscoveryInfo** dinfo, IServerInfo** sinfo,
                                               IDataBaseInfo** dbinfo) {
  IServerInfo* lsinfo = nullptr;
  common::Error er = serverInfo(&lsinfo);
  if (er && er->isError()) {
    return er;
  }

  IDataBaseInfo* ldbinfo = nullptr;
  er = currentDataBaseInfo(&ldbinfo);
  if (er && er->isError()) {
    delete lsinfo;
    return er;
  }

  uint32_t version = lsinfo->version();
  if (version < PROJECT_VERSION_CHECK(3,0,0)) {
    *sinfo = lsinfo;
    *dbinfo = ldbinfo;
    return common::Error(); //not error server not support cluster command
  }

  FastoObjectIPtr root = FastoObject::createRoot(GET_SERVER_TYPE);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root,
                                                        GET_SERVER_TYPE, common::Value::C_INNER);
  er = execute(cmd);

  if (er && er->isError()) {
    *sinfo = lsinfo;
    *dbinfo = ldbinfo;
    return common::Error(); //not error serverInfo is valid
  }

  auto ch = cmd->childrens();
  if (ch.size()) {
    FastoObject* obj = ch[0];
    if (obj) {
      common::Value::Type t = obj->type();
      if (t == common::Value::TYPE_STRING) {
        *dinfo = makeOwnRedisDiscoveryInfo(obj);
      }
    }
  }

  *sinfo = lsinfo;
  *dbinfo = ldbinfo;
  return er;
}

common::Error RedisDriver::currentDataBaseInfo(IDataBaseInfo** info) {
  return impl_->select(impl_->config_.dbnum, info);
}

void RedisDriver::handleConnectEvent(events::ConnectRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ConnectResponceEvent::value_type res(ev->value());
  RedisConnectionSettings* set = dynamic_cast<RedisConnectionSettings*>(settings_.get());
  if (set) {
    impl_->config_ = set->info();
    impl_->sinfo_ = set->sshInfo();
    notifyProgress(sender, 25);
    common::Error er = impl_->connect(false);
    if (er && er->isError()) {
      res.setErrorInfo(er);
    }
    notifyProgress(sender, 75);
  } else {
    NOTREACHED();
  }
  reply(sender, new events::ConnectResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleProcessCommandLineArgs(events::ProcessConfigArgsRequestEvent* ev) {
  /* Latency mode */
  if (impl_->config_.latency_mode) {
    latencyMode(ev);
  }

  /* Slave mode */
  if (impl_->config_.slave_mode) {
    slaveMode(ev);
  }

  /* Get RDB mode. */
  if (impl_->config_.getrdb_mode) {
    getRDBMode(ev);
  }

  /* Find big keys */
  if (impl_->config_.bigkeys) {
    findBigKeysMode(ev);
  }

  /* Stat mode */
  if (impl_->config_.stat_mode) {
    if (impl_->config_.interval == 0) {
      impl_->config_.interval = 1000000;
    }
    statMode(ev);
  }

  /* Scan mode */
  if (impl_->config_.scan_mode) {
    scanMode(ev);
  }

  interacteveMode(ev);

  QObject* sender = ev->sender();
  events::ProcessConfigArgsResponceEvent::value_type res(ev->value());
  reply(sender, new events::ProcessConfigArgsResponceEvent(this, res));
}

void RedisDriver::handleShutdownEvent(events::ShutDownRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ShutDownResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  FastoObjectIPtr root = FastoObject::createRoot(SHUTDOWN);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, SHUTDOWN, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ShutDownResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleBackupEvent(events::BackupRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::BackupResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  FastoObjectIPtr root = FastoObject::createRoot(BACKUP);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, BACKUP, common::Value::C_INNER);
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

void RedisDriver::handleExportEvent(events::ExportRequestEvent* ev) {
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

void RedisDriver::handleChangePasswordEvent(events::ChangePasswordRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangePasswordResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  std::string patternResult = common::MemSPrintf(SET_PASSWORD_1ARGS_S, res.new_password);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, patternResult, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  notifyProgress(sender, 75);
  reply(sender, new events::ChangePasswordResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleChangeMaxConnectionEvent(events::ChangeMaxConnectionRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangeMaxConnectionResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 25);
  std::string patternResult = common::MemSPrintf(SET_MAX_CONNECTIONS_1ARGS_I, res.max_connection);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, patternResult, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  notifyProgress(sender, 75);
  reply(sender, new events::ChangeMaxConnectionResponceEvent(this, res));
  notifyProgress(sender, 100);
}

common::Error RedisDriver::interacteveMode(events::ProcessConfigArgsRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::EnterModeEvent::value_type res(this, InteractiveMode);
  reply(sender, new events::EnterModeEvent(this, res));

  events::LeaveModeEvent::value_type res2(this, InteractiveMode);
  reply(sender, new events::LeaveModeEvent(this, res2));
  notifyProgress(sender, 100);
  return common::Error();
}

common::Error RedisDriver::latencyMode(events::ProcessConfigArgsRequestEvent* ev) {
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

common::Error RedisDriver::slaveMode(events::ProcessConfigArgsRequestEvent* ev) {
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

common::Error RedisDriver::getRDBMode(events::ProcessConfigArgsRequestEvent* ev) {
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

common::Error RedisDriver::findBigKeysMode(events::ProcessConfigArgsRequestEvent* ev) {
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

common::Error RedisDriver::statMode(events::ProcessConfigArgsRequestEvent* ev) {
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

common::Error RedisDriver::scanMode(events::ProcessConfigArgsRequestEvent* ev) {
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

void RedisDriver::handleExecuteEvent(events::ExecuteRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ExecuteResponceEvent::value_type res(ev->value());
  const char* inputLine = common::utils::c_strornull(res.text);

  common::Error er;
  if (inputLine) {
    size_t length = strlen(inputLine);
    int offset = 0;
    RootLocker lock = make_locker(sender, inputLine);
    FastoObjectIPtr obj = lock.root();
    double step = 100.0f / length;
    for (size_t n = 0; n < length; ++n) {
      if (isInterrupted()) {
        er.reset(new common::ErrorValue("Interrupted exec.", common::ErrorValue::E_INTERRUPTED));
        res.setErrorInfo(er);
        break;
      }

      if (inputLine[n] == '\n' || n == length - 1) {
        notifyProgress(sender, step * n);
        char command[128] = {0};
        if (n == length - 1) {
          strcpy(command, inputLine + offset);
        } else {
          strncpy(command, inputLine + offset, n - offset);
        }

        offset = n + 1;
        FastoObjectCommand* cmd = createCommand<RedisCommand>(obj, command,
                                                              common::Value::C_USER);
        er = execute(cmd);
        if (er && er->isError()) {
          res.setErrorInfo(er);
          break;
        }
      }
    }
  } else {
    er.reset(new common::ErrorValue("Empty command line.", common::ErrorValue::E_ERROR));
    res.setErrorInfo(er);
  }

  reply(sender, new events::ExecuteResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleCommandRequestEvent(events::CommandRequestEvent* ev) {
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
  FastoObjectCommand* cmd = createCommand<RedisCommand>(obj, cmdtext, common::Value::C_INNER);
  notifyProgress(sender, 50);
  er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  reply(sender, new events::CommandResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleDisconnectEvent(events::DisconnectRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::DisconnectResponceEvent::value_type res(ev->value());
  notifyProgress(sender, 50);

  common::Error er = impl_->disconnect();
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }

  reply(sender, new events::DisconnectResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::LoadDatabasesInfoResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(GET_DATABASES);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, GET_DATABASES,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    FastoObject::child_container_t rchildrens = cmd->childrens();
    if (rchildrens.size()) {
      DCHECK_EQ(rchildrens.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(rchildrens[0]);
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
            int countDb = common::convertFromString<int>(scountDb);
            if (countDb > 0) {
              for (size_t i = 0; i < countDb; ++i) {
                IDataBaseInfoSPtr dbInf(new RedisDataBaseInfo(common::convertToString(i), false, 0));
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

void RedisDriver::handleLoadDatabaseContentEvent(events::LoadDatabaseContentRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::LoadDatabaseContentResponceEvent::value_type res(ev->value());
  std::string patternResult = common::MemSPrintf(GET_KEYS_PATTERN_3ARGS_ISI, res.cursor_in,
                                                 res.pattern, res.count_keys);
  FastoObjectIPtr root = FastoObject::createRoot(patternResult);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, patternResult, common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    FastoObject::child_container_t rchildrens = cmd->childrens();
    if (rchildrens.size()) {
      DCHECK_EQ(rchildrens.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(rchildrens[0]);
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

      res.cursor_out = common::convertFromString<uint32_t>(cursor);

      rchildrens = array->childrens();
      if (!rchildrens.size()) {
        goto done;
      }

      FastoObject* obj = rchildrens[0];
      FastoObjectArray* arr = dynamic_cast<FastoObjectArray*>(obj);
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
          NDbKValue ress(k, NValue());
          cmds.push_back(createCommandFast("TYPE " + ress.keyString(), common::Value::C_INNER));
          cmds.push_back(createCommandFast("TTL " + ress.keyString(), common::Value::C_INNER));
          res.keys.push_back(ress);
        }
      }

      er = impl_->executeAsPipeline(cmds);
      if (er && er->isError()) {
        goto done;
      }

      for (size_t i = 0; i < res.keys.size(); ++i) {
        FastoObjectIPtr cmdType = cmds[i*2];
        FastoObject::child_container_t tchildrens = cmdType->childrens();
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

        FastoObjectIPtr cmdType2 = cmds[i*2+1];
        tchildrens = cmdType2->childrens();
        if (tchildrens.size()) {
          DCHECK_EQ(tchildrens.size(), 1);
          if (tchildrens.size() == 1) {
            FastoObject* fttl = tchildrens[0];
            common::Value* vttl = fttl->value();
            ttl_t ttl = 0;
            if (vttl->getAsInteger(&ttl)) {
                res.keys[i].setTTL(ttl);
            }
          }
        }
      }

      impl_->dbsize(&res.dbsize);
    }
  }
done:
  notifyProgress(sender, 75);
  reply(sender, new events::LoadDatabaseContentResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleClearDatabaseEvent(events::ClearDatabaseRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ClearDatabaseResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(REDIS_FLUSHDB);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, REDIS_FLUSHDB,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ClearDatabaseResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::SetDefaultDatabaseResponceEvent::value_type res(ev->value());
  std::string setDefCommand = common::MemSPrintf(SET_DEFAULT_DATABASE_PATTERN_1ARGS_S,
                                                 res.inf->name());
  FastoObjectIPtr root = FastoObject::createRoot(setDefCommand);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, setDefCommand,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  }
  notifyProgress(sender, 75);
  reply(sender, new events::SetDefaultDatabaseResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleLoadServerPropertyEvent(events::ServerPropertyInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ServerPropertyInfoResponceEvent::value_type res(ev->value());
  FastoObjectIPtr root = FastoObject::createRoot(GET_PROPERTY_SERVER);
  notifyProgress(sender, 50);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root,
                                                        GET_PROPERTY_SERVER,
                                                        common::Value::C_INNER);
  common::Error er = execute(cmd);
  if (er && er->isError()) {
    res.setErrorInfo(er);
  } else {
    FastoObject::child_container_t ch = cmd->childrens();
    if (ch.size()) {
      DCHECK_EQ(ch.size(), 1);
      FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(ch[0]);
      if (array) {
        res.info = makeServerProperty(array);
      }
    }
  }
  notifyProgress(sender, 75);
  reply(sender, new events::ServerPropertyInfoResponceEvent(this, res));
  notifyProgress(sender, 100);
}

void RedisDriver::handleServerPropertyChangeEvent(events::ChangeServerPropertyInfoRequestEvent* ev) {
  QObject* sender = ev->sender();
  notifyProgress(sender, 0);
  events::ChangeServerPropertyInfoResponceEvent::value_type res(ev->value());

  notifyProgress(sender, 50);
  std::string changeRequest = "CONFIG SET " + res.new_item.first + " " + res.new_item.second;
  FastoObjectIPtr root = FastoObject::createRoot(changeRequest);
  FastoObjectCommand* cmd = createCommand<RedisCommand>(root, changeRequest,
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

common::Error RedisDriver::commandDeleteImpl(CommandDeleteKey* command,
                                             std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  NDbKValue key = command->key();
  *cmdstring = common::MemSPrintf(DELETE_KEY_PATTERN_1ARGS_S, key.keyString());
  return common::Error();
}

common::Error RedisDriver::commandLoadImpl(CommandLoadKey* command, std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  if (key.type() == common::Value::TYPE_ARRAY) {
    patternResult = common::MemSPrintf(GET_KEY_LIST_PATTERN_1ARGS_S, key.keyString());
  } else if (key.type() == common::Value::TYPE_SET) {
    patternResult = common::MemSPrintf(GET_KEY_SET_PATTERN_1ARGS_S, key.keyString());
  } else if (key.type() == common::Value::TYPE_ZSET) {
    patternResult = common::MemSPrintf(GET_KEY_ZSET_PATTERN_1ARGS_S, key.keyString());
  } else if (key.type() == common::Value::TYPE_HASH) {
    patternResult = common::MemSPrintf(GET_KEY_HASH_PATTERN_1ARGS_S, key.keyString());
  } else {
    patternResult = common::MemSPrintf(GET_KEY_PATTERN_1ARGS_S, key.keyString());
  }

  *cmdstring = patternResult;
  return common::Error();
}

common::Error RedisDriver::commandCreateImpl(CommandCreateKey* command,
                                             std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  NValue val = command->value();
  common::Value* rval = val.get();
  std::string key_str = key.keyString();
  std::string value_str = common::convertToString(rval, " ");
  common::Value::Type t = key.type();
  if (t == common::Value::TYPE_ARRAY) {
    patternResult = common::MemSPrintf(SET_KEY_LIST_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_SET) {
    patternResult = common::MemSPrintf(SET_KEY_SET_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_ZSET) {
    patternResult = common::MemSPrintf(SET_KEY_ZSET_PATTERN_2ARGS_SS, key_str, value_str);
  } else if (t == common::Value::TYPE_HASH) {
    patternResult = common::MemSPrintf(SET_KEY_HASH_PATTERN_2ARGS_SS, key_str, value_str);
  } else {
    patternResult = common::MemSPrintf(SET_KEY_PATTERN_2ARGS_SS, key_str, value_str);
  }

  *cmdstring = patternResult;
  return common::Error();
}

common::Error RedisDriver::commandChangeTTLImpl(CommandChangeTTL* command,
                                                std::string* cmdstring) const {
  if (!command || !cmdstring) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string patternResult;
  NDbKValue key = command->key();
  ttl_t new_ttl = command->newTTL();
  if (new_ttl == -1) {
    patternResult = common::MemSPrintf(PERSIST_KEY_1ARGS_S, key.keyString());
  } else {
    patternResult = common::MemSPrintf(CHANGE_TTL_2ARGS_SI, key.keyString(), new_ttl);
  }

  *cmdstring = patternResult;
  return common::Error();
}

IServerInfoSPtr RedisDriver::makeServerInfoFromString(const std::string& val) {
  IServerInfoSPtr res(makeRedisServerInfo(val));
  return res;
}

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
