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

#include "core/database/idatabase_info.h"

#include <string>  // for string

namespace fastonosql {
namespace core {

IDataBaseInfo::IDataBaseInfo(const std::string& name,
                             bool isDefault,
                             connectionTypes type,
                             size_t dbkcount,
                             const keys_container_t& keys)
    : name_(name), is_default_(isDefault), db_kcount_(dbkcount), keys_(keys), type_(type) {}

IDataBaseInfo::~IDataBaseInfo() {}

connectionTypes IDataBaseInfo::Type() const {
  return type_;
}

std::string IDataBaseInfo::Name() const {
  return name_;
}

size_t IDataBaseInfo::DBKeysCount() const {
  return db_kcount_;
}

void IDataBaseInfo::SetDBKeysCount(size_t size) {
  db_kcount_ = size;
}

size_t IDataBaseInfo::LoadedKeysCount() const {
  return keys_.size();
}

bool IDataBaseInfo::IsDefault() const {
  return is_default_;
}

void IDataBaseInfo::SetIsDefault(bool isDef) {
  is_default_ = isDef;
}

void IDataBaseInfo::SetKeys(const keys_container_t& keys) {
  keys_ = keys;
}

void IDataBaseInfo::ClearKeys() {
  keys_.clear();
}

IDataBaseInfo::keys_container_t IDataBaseInfo::Keys() const {
  return keys_;
}

}  // namespace core
}  // namespace fastonosql
