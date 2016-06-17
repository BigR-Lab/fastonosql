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

#include "core/redis/connection_settings.h"

#include <string>

#include "common/utils.h"

namespace fastonosql {
namespace core {
namespace redis {

ConnectionSettings::ConnectionSettings(const connection_path_t& connectionName)
  : IConnectionSettingsRemoteSSH(connectionName, REDIS), info_() {
}

void ConnectionSettings::setHost(const common::net::hostAndPort& host) {
  info_.host = host;
}

common::net::hostAndPort ConnectionSettings::host() const {
  return info_.host;
}

void ConnectionSettings::setCommandLine(const std::string& line) {
  info_ = common::ConvertFromString<Config>(line);
}

std::string ConnectionSettings::commandLine() const {
  return common::ConvertToString(info_);
}

Config ConnectionSettings::info() const {
  return info_;
}

void ConnectionSettings::setInfo(const Config &info) {
  info_ =  info;
}

ConnectionSettings* ConnectionSettings::clone() const {
  ConnectionSettings* red = new ConnectionSettings(*this);
  return red;
}

}  // namespace redis
}  // namespace core
}  // namespace fastonosql
