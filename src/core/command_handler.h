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

#include <vector>

#include "common/error.h"               // for Error
#include "common/macros.h"              // for WARN_UNUSED_RESULT
#include "core/command_holder.h"        // for CommandHolder

namespace fastonosql {
namespace core {

class CommandHandler {
 public:
  typedef CommandHolder commands_t;
  explicit CommandHandler(const std::vector<commands_t>& commands);
  common::Error execute(int argc, char** argv, FastoObject* out) WARN_UNUSED_RESULT;

  static common::Error notSupported(const char* cmd);
 private:
  const std::vector<commands_t> commands_;
};

}  // namespace core
}  // namespace fastonosql
