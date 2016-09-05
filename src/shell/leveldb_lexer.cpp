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

#include "shell/leveldb_lexer.h"

#include "core/leveldb/db_connection.h"  // for leveldbCommands, etc

namespace fastonosql {
namespace shell {

LeveldbApi::LeveldbApi(QsciLexer* lexer)
    : BaseQsciApiCommandHolder(core::leveldb::leveldbCommands, lexer) {}

LeveldbLexer::LeveldbLexer(QObject* parent)
    : BaseQsciLexerCommandHolder(core::leveldb::leveldbCommands, parent) {
  setAPIs(new LeveldbApi(this));
}

const char* LeveldbLexer::language() const {
  return "LevelDB";
}

const char* LeveldbLexer::version() const {
  return core::leveldb::DBConnection::versionApi();
}

const char* LeveldbLexer::basedOn() const {
  return "leveldb";
}

}  // namespace shell
}  // namespace fastonosql
