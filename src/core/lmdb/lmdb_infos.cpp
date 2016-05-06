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

#include "core/lmdb/lmdb_infos.h"

#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#define MARKER "\r\n"

namespace fastonosql {
namespace core {
namespace {

const std::vector<Field> lmdbCommonFields = {
  Field(LMDB_FILE_NAME_LABEL, common::Value::TYPE_STRING)
};

}  // namespace

template<>
std::vector<common::Value::Type> DBTraits<LMDB>::supportedTypes() {
  return  {
              common::Value::TYPE_BOOLEAN,
              common::Value::TYPE_INTEGER,
              common::Value::TYPE_UINTEGER,
              common::Value::TYPE_DOUBLE,
              common::Value::TYPE_STRING,
              common::Value::TYPE_ARRAY
          };
}

template<>
std::vector<std::string> DBTraits<LMDB>::infoHeaders() {
  return { LMDB_STATS_LABEL };
}

template<>
std::vector<std::vector<Field> > DBTraits<LMDB>::infoFields() {
  return { lmdbCommonFields };
}
namespace lmdb {

LmdbServerInfo::Stats::Stats()
  : file_name() {
}

LmdbServerInfo::Stats::Stats(const std::string& common_text) {
  size_t pos = 0;
  size_t start = 0;

  while ((pos = common_text.find(MARKER, start)) != std::string::npos) {
    std::string line = common_text.substr(start, pos-start);
    size_t delem = line.find_first_of(':');
    std::string field = line.substr(0, delem);
    std::string value = line.substr(delem + 1);
    if (field == LMDB_FILE_NAME_LABEL) {
      file_name = value;
    }
    start = pos + 2;
  }
}

common::Value* LmdbServerInfo::Stats::valueByIndex(unsigned char index) const {
  switch (index) {
  case 0:
    return new common::StringValue(file_name);
  default:
    NOTREACHED();
    break;
  }
  return nullptr;
}

LmdbServerInfo::LmdbServerInfo()
  : IServerInfo(LMDB) {
}

LmdbServerInfo::LmdbServerInfo(const Stats& stats)
  : IServerInfo(LMDB), stats_(stats) {
}

common::Value* LmdbServerInfo::valueByIndexes(unsigned char property, unsigned char field) const {
  switch (property) {
  case 0:
    return stats_.valueByIndex(field);
  default:
    NOTREACHED();
    break;
  }
  return nullptr;
}

std::ostream& operator<<(std::ostream& out, const LmdbServerInfo::Stats& value) {
  return out << LMDB_FILE_NAME_LABEL ":" << value.file_name << MARKER;
}

std::ostream& operator<<(std::ostream& out, const LmdbServerInfo& value) {
  return out << value.toString();
}

LmdbServerInfo* makeLmdbServerInfo(const std::string& content) {
  if (content.empty()) {
    return nullptr;
  }

  LmdbServerInfo* result = new LmdbServerInfo;

  const std::vector<std::string> headers = DBTraits<LMDB>::infoHeaders();
  std::string word;
  DCHECK_EQ(headers.size(), 1);

  for (size_t i = 0; i < content.size(); ++i) {
    word += content[i];
    if (word == headers[0]) {
      std::string part = content.substr(i + 1);
      result->stats_ = LmdbServerInfo::Stats(part);
      break;
    }
  }

  return result;
}


std::string LmdbServerInfo::toString() const {
  std::stringstream str;
  str << LMDB_STATS_LABEL MARKER << stats_;
  return str.str();
}

uint32_t LmdbServerInfo::version() const {
  return 0;
}

LmdbServerInfo* makeLmdbServerInfo(FastoObject* root) {
  std::string content = common::convertToString(root);
  return makeLmdbServerInfo(content);
}

}  // namespace lmdb
}  // namespace core
}  // namespace fastonosql
