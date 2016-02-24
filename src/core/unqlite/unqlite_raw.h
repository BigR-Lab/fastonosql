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

extern "C" {
  #include <unqlite.h>
}

#include <string>

#include "core/unqlite/unqlite_settings.h"
#include "core/unqlite/unqlite_config.h"
#include "core/unqlite/unqlite_infos.h"

namespace fastonosql {
namespace unqlite {

common::Error testConnection(UnqliteConnectionSettings* settings);

class UnqliteRaw
    : public CommandHandler {
 public:
  UnqliteRaw();
  ~UnqliteRaw();

  static const char* versionApi();

  bool isConnected() const;
  common::Error connect();
  common::Error disconnect();

  common::Error info(const char* args, UnqliteServerInfo::Stats* statsout);
  common::Error dbsize(size_t& size);
  common::Error get(const std::string& key, std::string* ret_val);
  common::Error put(const std::string& key, const std::string& value);
  common::Error del(const std::string& key);
  common::Error keys(const std::string& key_start, const std::string& key_end,
                     uint64_t limit, std::vector<std::string>* ret);

  unqliteConfig config_;
 private:
  struct unqlite* unqlite_;
};

common::Error put(CommandHandler* handler, int argc, char** argv, FastoObject* out);
common::Error get(CommandHandler* handler, int argc, char** argv, FastoObject* out);
common::Error del(CommandHandler* handler, int argc, char** argv, FastoObject* out);
common::Error keys(CommandHandler* handler, int argc, char** argv, FastoObject* out);
common::Error info(CommandHandler* handler, int argc, char** argv, FastoObject* out);
common::Error dbsize(CommandHandler* handler, int argc, char** argv, FastoObject* out);

static const std::vector<CommandHolder> unqliteCommands = {
  CommandHolder("PUT", "<key> <value>",
              "Set the value of a key.",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 2, 0, &put),
  CommandHolder("GET", "<key>",
              "Get the value of a key.",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 1, 0, &get),
  CommandHolder("DEL", "<key>",
              "Delete key.",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 1, 0, &del),
  CommandHolder("KEYS", "<key_start> <key_end> <limit>",
              "Find all keys matching the given limits.",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 3, 0, &keys),
  CommandHolder("INFO", "<args>",
              "These command return database information.",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 1, 0, &info),

  CommandHolder("DBSIZE", "-",
              "Return the number of keys in the selected database",
              UNDEFINED_SINCE, UNDEFINED_EXAMPLE_STR, 0, 0, &dbsize)
};

}  // namespace unqlite
}  // namespace fastonosql
