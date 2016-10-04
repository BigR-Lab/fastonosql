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
#include <memory>  // for shared_ptr

#include <common/error.h>   // for Error
#include <common/macros.h>  // for WARN_UNUSED_RESULT
#include <common/value.h>   // for Value, Value::Type

#include "core/db_key.h"  // for key_t, key_and_value_t, ttl_t

namespace fastonosql {
namespace core {

class ICommandTranslator {
 public:
  common::Error createKeyCommand(const key_and_value_t& key,
                                 std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error loadKeyCommand(const key_t& key,
                               common::Value::Type type,
                               std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error deleteKeyCommand(const key_t& key, std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error renameKeyCommand(const key_t& key,
                                 const std::string& new_name,
                                 std::string* cmdstring) const WARN_UNUSED_RESULT;
  common::Error changeKeyTTLCommand(const key_t& key,
                                    ttl_t ttl,
                                    std::string* cmdstring) const WARN_UNUSED_RESULT;

 private:
  virtual common::Error createKeyCommandImpl(const key_and_value_t& key,
                                             std::string* cmdstring) const = 0;
  virtual common::Error loadKeyCommandImpl(const key_t& key,
                                           common::Value::Type type,
                                           std::string* cmdstring) const = 0;
  virtual common::Error deleteKeyCommandImpl(const key_t& key, std::string* cmdstring) const = 0;
  virtual common::Error renameKeyCommandImpl(const key_t& key,
                                             const std::string& new_name,
                                             std::string* cmdstring) const = 0;
  virtual common::Error changeKeyTTLCommandImpl(const key_t& key,
                                                ttl_t ttl,
                                                std::string* cmdstring) const = 0;
};

typedef common::shared_ptr<ICommandTranslator> translator_t;

}  // namespace core
}  // namespace fastonosql
