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

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int32_t

#include <string>  // for string
#include <vector>  // for vector

#include <common/value.h>  // for Value, Value::Type, etc

#define NO_TTL -1

namespace fastonosql {
namespace core {

typedef int32_t ttl_t;

class KeyInfo {
 public:
  typedef std::vector<std::string> splited_namespaces_t;
  KeyInfo(const splited_namespaces_t& splited_namespaces_and_key, const std::string& ns_separator);

  std::string Key() const;
  bool HasNamespace() const;
  std::string Nspace() const;
  std::string JoinNamespace(size_t pos) const;
  size_t NspaceSize() const;

 private:
  splited_namespaces_t splited_namespaces_and_key_;
  std::string ns_separator_;
};

class NKey {
 public:
  NKey();
  explicit NKey(const std::string& key, ttl_t ttl_sec = NO_TTL);
  KeyInfo Info(const std::string& ns_separator) const;

  std::string Key() const;
  void SetKey(const std::string& key);

  ttl_t TTL() const;
  void SetTTL(ttl_t ttl);

  bool Equals(const NKey& other) const;

 private:
  std::string key_;
  ttl_t ttl_;
};

typedef std::vector<NKey> NKeys;
typedef common::ValueSPtr NValue;

class NDbKValue {
 public:
  NDbKValue();
  NDbKValue(const NKey& key, NValue value);

  NKey Key() const;
  NValue Value() const;
  common::Value::Type Type() const;

  void SetTTL(ttl_t ttl);
  void SetKey(const NKey& key);
  void SetValue(NValue value);

  std::string KeyString() const;
  std::string ValueString() const;

  bool Equals(const NDbKValue& other) const;

 private:
  NKey key_;
  NValue value_;
};

typedef std::vector<NDbKValue> NDbKValues;

}  // namespace core
}  // namespace fastonosql
