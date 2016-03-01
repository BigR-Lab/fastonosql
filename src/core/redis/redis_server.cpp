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

#include "core/redis/redis_server.h"

#include "core/redis/redis_driver.h"
#include "core/redis/redis_database.h"

namespace fastonosql {
namespace redis {

RedisServer::RedisServer(IConnectionSettingsBaseSPtr settings)
  : IServerRemote(new RedisDriver(settings)), role_(SLAVE) {
}

serverTypes RedisServer::role() const {
  return role_;
}

common::net::hostAndPort RedisServer::host() const {
  RedisDriver* const rdrv = static_cast<RedisDriver* const>(drv_);
  return rdrv->host();
}

IDatabaseSPtr RedisServer::createDatabase(IDataBaseInfoSPtr info) {
  return IDatabaseSPtr(new RedisDatabase(shared_from_this(), info));
}

void RedisServer::handleDiscoveryInfoResponceEvent(events::DiscoveryInfoResponceEvent* ev) {
  auto v = ev->value();
  common::Error er = v.errorInfo();
  if (!er) {
    RedisServerInfo * rinf = dynamic_cast<RedisServerInfo*>(v.sinfo.get());
    CHECK(rinf);
    if (rinf->replication_.role_ == "master") {
      role_ = MASTER;
    } else if(rinf->replication_.role_ == "slave") {
      role_ = SLAVE;
    } else {
      NOTREACHED();
    }
  }
  IServer::handleDiscoveryInfoResponceEvent(ev);
}

}  // namespace redis
}  // namespace fastonosql
