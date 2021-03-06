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

#include <stddef.h>  // for size_t

#include <string>  // for string
#include <vector>  // for vector

#include <common/error.h>   // for Error
#include <common/macros.h>  // for PROJECT_VERSION_GENERATE, etc

#include "core/command_info.h"      // for UNDEFINED_EXAMPLE_STR, etc
#include "core/command_holder.h"    // for CommandHolder, etc
#include "core/connection_types.h"  // for connectionTypes::REDIS
#include "core/db_key.h"            // for NDbKValue, NKey, etc
#include "core/ssh_info.h"          // for SSHInfo

#include "core/internal/cdb_connection.h"  // for CDBConnection

#include "core/redis/config.h"  // for Config
#include "core/server/iserver_info.h"

#include "global/global.h"  // for FastoObject (ptr only), etc

namespace fastonosql {
namespace core {
class CDBConnectionClient;
}
}
namespace fastonosql {
namespace core {
class CommandHandler;
}
}
namespace fastonosql {
namespace core {
class IDataBaseInfo;
}
}
namespace fastonosql {
namespace core {
namespace redis {
class ConnectionSettings;
}
}
}
struct redisContext;  // lines 49-49
struct redisReply;    // lines 50-50

#define INFO_REQUEST "INFO"
#define GET_SERVER_TYPE "CLUSTER NODES"
#define GET_SENTINEL_MASTERS "SENTINEL MASTERS"
#define GET_SENTINEL_SLAVES_PATTERN_1ARGS_S "SENTINEL SLAVES %s"

#define LATENCY_REQUEST "LATENCY"
#define RDM_REQUEST "RDM"
#define SYNC_REQUEST "SYNC"
#define FIND_BIG_KEYS_REQUEST "FIND_BIG_KEYS"
#define STAT_MODE_REQUEST "STAT"
#define SCAN_MODE_REQUEST "SCAN"

namespace fastonosql {
namespace core {
namespace redis {

typedef redisContext NativeConnection;
struct RConfig : public Config {
  explicit RConfig(const Config& config, const SSHInfo& sinfo);
  RConfig();

  SSHInfo ssh_info;
};

common::Error CreateConnection(const RConfig& config, NativeConnection** context);
common::Error CreateConnection(ConnectionSettings* settings, NativeConnection** context);
common::Error TestConnection(ConnectionSettings* settings);

common::Error DiscoveryClusterConnection(ConnectionSettings* settings,
                                         std::vector<ServerDiscoveryClusterInfoSPtr>* infos);
common::Error DiscoverySentinelConnection(ConnectionSettings* settings,
                                          std::vector<ServerDiscoverySentinelInfoSPtr>* infos);

class DBConnection : public core::internal::CDBConnection<NativeConnection, RConfig, REDIS> {
 public:
  typedef core::internal::CDBConnection<NativeConnection, RConfig, REDIS> base_class;
  explicit DBConnection(CDBConnectionClient* client);

  bool IsAuthenticated() const;

  common::Error Connect(const config_t& config);

  common::Error LatencyMode(FastoObject* out) WARN_UNUSED_RESULT;
  common::Error SlaveMode(FastoObject* out) WARN_UNUSED_RESULT;
  common::Error GetRDB(FastoObject* out) WARN_UNUSED_RESULT;
  common::Error DBkcount(size_t* size) WARN_UNUSED_RESULT;
  common::Error FindBigKeys(FastoObject* out) WARN_UNUSED_RESULT;
  common::Error StatMode(FastoObject* out) WARN_UNUSED_RESULT;
  common::Error ScanMode(FastoObject* out) WARN_UNUSED_RESULT;

  common::Error ExecuteAsPipeline(const std::vector<FastoObjectCommandIPtr>& cmds)
      WARN_UNUSED_RESULT;

  common::Error CommonExec(int argc, const char** argv, FastoObject* out) WARN_UNUSED_RESULT;
  common::Error Auth(const std::string& password) WARN_UNUSED_RESULT;
  common::Error Help(int argc, const char** argv, FastoObject* out) WARN_UNUSED_RESULT;
  common::Error Monitor(int argc,
                        const char** argv,
                        FastoObject* out) WARN_UNUSED_RESULT;  // interrupt
  common::Error Subscribe(int argc,
                          const char** argv,
                          FastoObject* out) WARN_UNUSED_RESULT;  // interrupt

  common::Error Lrange(const NKey& key, int start, int stop, NDbKValue* loaded_key);
  common::Error Smembers(const NKey& key, NDbKValue* loaded_key);
  common::Error Zrange(const NKey& key,
                       int start,
                       int stop,
                       bool withscores,
                       NDbKValue* loaded_key);
  common::Error Hgetall(const NKey& key, NDbKValue* loaded_key);

 private:
  virtual common::Error SelectImpl(const std::string& name, IDataBaseInfo** info) override;
  virtual common::Error DeleteImpl(const NKeys& keys, NKeys* deleted_keys) override;
  virtual common::Error SetImpl(const NDbKValue& key, NDbKValue* added_key) override;
  virtual common::Error GetImpl(const NKey& key, NDbKValue* loaded_key) override;
  virtual common::Error RenameImpl(const NKey& key, const std::string& new_key) override;
  virtual common::Error SetTTLImpl(const NKey& key, ttl_t ttl) override;
  virtual common::Error QuitImpl() override;

  common::Error SendSync(unsigned long long* payload) WARN_UNUSED_RESULT;
  common::Error SendScan(unsigned long long* it, redisReply** out) WARN_UNUSED_RESULT;
  common::Error GetKeyTypes(redisReply* keys, int* types) WARN_UNUSED_RESULT;
  common::Error GetKeySizes(redisReply* keys,
                            int* types,
                            unsigned long long* sizes) WARN_UNUSED_RESULT;

  common::Error CliFormatReplyRaw(FastoObjectArray* ar, redisReply* r) WARN_UNUSED_RESULT;
  common::Error CliFormatReplyRaw(FastoObject* out, redisReply* r) WARN_UNUSED_RESULT;
  common::Error CliReadReply(FastoObject* out) WARN_UNUSED_RESULT;

  bool isAuth_;
};

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
