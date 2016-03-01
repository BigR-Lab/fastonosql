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

#include "core/memcached/memcached_server.h"

#include "core/memcached/memcached_driver.h"
#include "core/memcached/memcached_database.h"

namespace fastonosql {
namespace memcached {

MemcachedServer::MemcachedServer(IConnectionSettingsBaseSPtr settings)
  : IServer(new MemcachedDriver(settings)) {
}

serverTypes MemcachedServer::role() const {
  return MASTER;
}

IDatabaseSPtr MemcachedServer::createDatabase(IDataBaseInfoSPtr info) {
  return IDatabaseSPtr(new MemcachedDatabase(shared_from_this(), info));
}

}  // namespace memcached
}  // namespace fastonosql
