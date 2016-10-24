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

#include <string>
#include <vector>

#include <common/net/types.h>

namespace fastonosql {
namespace core {

enum ConfigType { LOCAL, REMOTE };

// -d -ns
template <ConfigType ctype>
struct BaseConfig {
  BaseConfig() : delimiter("\n"), ns_separator(":") {}

  static ConfigType Type() { return ctype; }

  std::string delimiter;
  std::string ns_separator;
};

// -f -d -ns
struct LocalConfig : public BaseConfig<LOCAL> {
  explicit LocalConfig(const std::string& dbname);

  std::vector<std::string> Args() const;

  std::string dbname;
};

// -h -p -d -ns
struct RemoteConfig : public BaseConfig<REMOTE> {
  explicit RemoteConfig(const common::net::HostAndPort& host);

  std::vector<std::string> Args() const;

  common::net::HostAndPort host;
};

}  // namespace core
}  // namespace fastonosql
