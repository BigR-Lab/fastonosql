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

#include "core/leveldb/leveldb_raw.h"

#include <mutex>

#include "common/sprintf.h"

#define LEVELDB_HEADER_STATS    "                               Compactions\n"\
                                "Level  Files Size(MB) Time(sec) Read(MB) Write(MB)\n"\
                                "--------------------------------------------------\n"

namespace {

std::once_flag leveldb_version_once;
void leveldb_version_startup_function(char* version) {
  sprintf(version, "%d.%d", leveldb::kMajorVersion, leveldb::kMinorVersion);
}

}

namespace fastonosql {
namespace core {
template<>
common::Error DBAllocatorTraits<leveldb::LevelDBConnection, leveldb::Config>::connect(const leveldb::Config& config, leveldb::LevelDBConnection** hout) {
  leveldb::LevelDBConnection* context = nullptr;
  common::Error er = leveldb::createConnection(config, &context);
  if (er && er->isError()) {
    return er;
  }

  *hout = context;
  return common::Error();
}
template<>
common::Error DBAllocatorTraits<leveldb::LevelDBConnection, leveldb::Config>::disconnect(leveldb::LevelDBConnection** handle) {
  destroy(handle);
  return common::Error();
}
template<>
bool DBAllocatorTraits<leveldb::LevelDBConnection, leveldb::Config>::isConnected(leveldb::LevelDBConnection* handle) {
  if (!handle) {
    return false;
  }

  return true;
}
namespace leveldb {

common::Error createConnection(const Config& config, LevelDBConnection** context) {
  if (!context) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  DCHECK(*context == nullptr);
  ::leveldb::DB* lcontext = nullptr;
  auto st = ::leveldb::DB::Open(config.options, config.dbname, &lcontext);
  if (!st.ok()) {
    std::string buff = common::MemSPrintf("Fail connect to server: %s!", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }

  *context = lcontext;
  return common::Error();
}

common::Error createConnection(LeveldbConnectionSettings* settings, LevelDBConnection** context) {
  if (!settings) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  Config config = settings->info();
  return createConnection(config, context);
}

common::Error testConnection(LeveldbConnectionSettings* settings) {
  if (!settings) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  leveldb::LevelDBConnection* ldb = nullptr;
  common::Error er = createConnection(settings, &ldb);
  if (er && er->isError()) {
    return er;
  }

  delete ldb;
  return common::Error();
}

LeveldbRaw::LeveldbRaw()
  : CommandHandler(leveldbCommands), connection_() {
}

common::Error LeveldbRaw::connect(const config_t& config) {
  return connection_.connect(config);
}

common::Error LeveldbRaw::disconnect() {
  return connection_.disconnect();
}

bool LeveldbRaw::isConnected() const {
  return connection_.isConnected();
}

std::string LeveldbRaw::delimiter() const {
  return connection_.config_.delimiter;
}

std::string LeveldbRaw::nsSeparator() const {
  return connection_.config_.ns_separator;
}

LeveldbRaw::config_t LeveldbRaw::config() const {
  return connection_.config_;
}

const char* LeveldbRaw::versionApi() {
  static char leveldb_version[32] = {0};
  std::call_once(leveldb_version_once, leveldb_version_startup_function, leveldb_version);
  return leveldb_version;
}

common::Error LeveldbRaw::dbsize(size_t* size) {
  CHECK(isConnected());

  if (!size) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  ::leveldb::ReadOptions ro;
  ::leveldb::Iterator* it = connection_.handle_->NewIterator(ro);
  size_t sz = 0;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    sz++;
  }

  auto st = it->status();
  delete it;

  if (!st.ok()) {
    std::string buff = common::MemSPrintf("Couldn't determine DBSIZE error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }

  *size = sz;
  return common::Error();
}

common::Error LeveldbRaw::info(const char* args, ServerInfo::Stats* statsout) {
  CHECK(isConnected());

  if (!statsout) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  std::string rets;
  bool isok = connection_.handle_->GetProperty("leveldb.stats", &rets);
  if (!isok) {
    return common::make_error_value("info function failed", common::ErrorValue::E_ERROR);
  }

  ServerInfo::Stats lstats;
  if (rets.size() > sizeof(LEVELDB_HEADER_STATS)) {
    const char* retsc = rets.c_str() + sizeof(LEVELDB_HEADER_STATS);
    char* p2 = strtok((char*)retsc, " ");
    int pos = 0;
    while (p2) {
      switch (pos++) {
        case 0:
          lstats.compactions_level = atoi(p2);
          break;
        case 1:
          lstats.file_size_mb = atoi(p2);
          break;
        case 2:
          lstats.time_sec = atoi(p2);
          break;
        case 3:
          lstats.read_mb = atoi(p2);
          break;
        case 4:
          lstats.write_mb = atoi(p2);
          break;
        default:
          break;
      }
      p2 = strtok(0, " ");
    }
  }

  *statsout = lstats;
  return common::Error();
}

common::Error LeveldbRaw::set(const std::string& key, const std::string& value) {
  CHECK(isConnected());

  ::leveldb::WriteOptions wo;
  auto st = connection_.handle_->Put(wo, key, value);
  if (!st.ok()) {
    std::string buff = common::MemSPrintf("set function error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }

  return common::Error();
}

common::Error LeveldbRaw::get(const std::string& key, std::string* ret_val) {
  CHECK(isConnected());

  ::leveldb::ReadOptions ro;
  auto st = connection_.handle_->Get(ro, key, ret_val);
  if (!st.ok()) {
    std::string buff = common::MemSPrintf("get function error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }

  return common::Error();
}

common::Error LeveldbRaw::del(const std::string& key) {
  CHECK(isConnected());

  ::leveldb::WriteOptions wo;
  auto st = connection_.handle_->Delete(wo, key);
  if (!st.ok()) {
    std::string buff = common::MemSPrintf("del function error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }
  return common::Error();
}

common::Error LeveldbRaw::keys(const std::string& key_start, const std::string& key_end,
                   uint64_t limit, std::vector<std::string>* ret) {
  CHECK(isConnected());

  ::leveldb::ReadOptions ro;
  ::leveldb::Iterator* it = connection_.handle_->NewIterator(ro);  // keys(key_start, key_end, limit, ret);
  for (it->Seek(key_start); it->Valid() && it->key().ToString() < key_end; it->Next()) {
    std::string key = it->key().ToString();
    if (ret->size() <= limit) {
      ret->push_back(key);
    } else {
      break;
    }
  }

  auto st = it->status();
  delete it;

  if (!st.ok()) {
    std::string buff = common::MemSPrintf("Keys function error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }
  return common::Error();
}

common::Error LeveldbRaw::help(int argc, char** argv) {
  return notSupported("HELP");
}

common::Error LeveldbRaw::flushdb() {
  CHECK(isConnected());

  ::leveldb::ReadOptions ro;
  ::leveldb::WriteOptions wo;
  ::leveldb::Iterator* it = connection_.handle_->NewIterator(ro);
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    std::string key = it->key().ToString();
    auto st = connection_.handle_->Delete(wo, key);
    if (!st.ok()) {
      delete it;
      std::string buff = common::MemSPrintf("del function error: %s", st.ToString());
      return common::make_error_value(buff, common::ErrorValue::E_ERROR);
    }
  }

  auto st = it->status();
  delete it;

  if (!st.ok()) {
    std::string buff = common::MemSPrintf("Keys function error: %s", st.ToString());
    return common::make_error_value(buff, common::ErrorValue::E_ERROR);
  }
  return common::Error();
}

common::Error dbsize(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  size_t dbsize = 0;
  common::Error er = level->dbsize(&dbsize);
  if (!er) {
    common::FundamentalValue* val = common::Value::createUIntegerValue(dbsize);
    FastoObject* child = new FastoObject(out, val, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error info(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  ServerInfo::Stats statsout;
  common::Error er = level->info(argc == 1 ? argv[0] : nullptr, &statsout);
  if (!er) {
    ServerInfo linf(statsout);
    common::StringValue* val = common::Value::createStringValue(linf.toString());
    FastoObject* child = new FastoObject(out, val, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error set(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  common::Error er = level->set(argv[0], argv[1]);
  if (!er) {
    common::StringValue* val = common::Value::createStringValue("STORED");
    FastoObject* child = new FastoObject(out, val, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error get(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  std::string ret;
  common::Error er = level->get(argv[0], &ret);
  if (!er) {
    common::StringValue* val = common::Value::createStringValue(ret);
    FastoObject* child = new FastoObject(out, val, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error del(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  common::Error er = level->del(argv[0]);
  if (!er) {
    common::StringValue* val = common::Value::createStringValue("DELETED");
    FastoObject* child = new FastoObject(out, val, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error keys(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);

  std::vector<std::string> keysout;
  common::Error er = level->keys(argv[0], argv[1], atoll(argv[2]), &keysout);
  if (!er) {
    common::ArrayValue* ar = common::Value::createArrayValue();
    for (size_t i = 0; i < keysout.size(); ++i) {
      common::StringValue* val = common::Value::createStringValue(keysout[i]);
      ar->append(val);
    }
    FastoObjectArray* child = new FastoObjectArray(out, ar, level->delimiter(), level->nsSeparator());
    out->addChildren(child);
  }
  return er;
}

common::Error help(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);
  return level->help(argc - 1, argv + 1);
}

common::Error flushdb(CommandHandler* handler, int argc, char** argv, FastoObject* out) {
  LeveldbRaw* level = static_cast<LeveldbRaw*>(handler);
  return level->flushdb();
}

}  // namespace leveldb
}  // namespace core
}  // namespace fastonosql
