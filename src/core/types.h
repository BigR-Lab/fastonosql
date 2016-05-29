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
#include <string>

#include "common/net/net.h"

#include "core/connection_types.h"
#include "core/db_key.h"

namespace fastonosql {
namespace core {

struct ServerCommonInfo {
  ServerCommonInfo();

  std::string name;
  serverTypes type;
  common::net::hostAndPort host;
};

class ServerDiscoveryInfoBase {
 public:
  connectionTypes connectionType() const;
  serverTypes type() const;

  std::string name() const;
  void setName(const std::string& name);

  common::net::hostAndPort host() const;
  void setHost(const common::net::hostAndPort& host);

 protected:
  ServerDiscoveryInfoBase(connectionTypes ctype, const ServerCommonInfo& info);
  ~ServerDiscoveryInfoBase();

 private:
  const connectionTypes ctype_;
  ServerCommonInfo info_;
};

class ServerDiscoverySentinelInfo
  : public ServerDiscoveryInfoBase {
 protected:
  ServerDiscoverySentinelInfo(connectionTypes ctype, const ServerCommonInfo& info);
};

typedef common::shared_ptr<ServerDiscoverySentinelInfo> ServerDiscoverySentinelInfoSPtr;

class ServerDiscoveryClusterInfo
  : public ServerDiscoveryInfoBase{
 public:
  bool self() const;

 protected:
  ServerDiscoveryClusterInfo(connectionTypes ctype, const ServerCommonInfo& info, bool self);

 private:
  const bool self_;
};

typedef common::shared_ptr<ServerDiscoveryClusterInfo> ServerDiscoveryClusterInfoSPtr;

class IServerInfo {
 public:
  explicit IServerInfo(connectionTypes type);
  virtual ~IServerInfo();

  connectionTypes type() const;
  virtual std::string toString() const = 0;
  virtual uint32_t version() const = 0;
  virtual common::Value* valueByIndexes(unsigned char property, unsigned char field) const = 0;

 private:
  const connectionTypes type_;
};

typedef common::shared_ptr<IServerInfo> IServerInfoSPtr;

struct IStateField {
  virtual common::Value* valueByIndex(unsigned char index) const = 0;
};

struct Field {
  Field(const std::string& name, common::Value::Type type);

  bool isIntegral() const;
  std::string name;
  common::Value::Type type;
};

template<connectionTypes ct>
struct DBTraits {
  static std::vector<common::Value::Type> supportedTypes();
  static std::vector<std::string> infoHeaders();
  static std::vector<std::vector<Field> > infoFields();
};

std::vector<common::Value::Type> supportedTypesFromType(connectionTypes type);
std::vector<std::string> infoHeadersFromType(connectionTypes type);
std::vector<std::vector<Field> > infoFieldsFromType(connectionTypes type);

struct ServerInfoSnapShoot {
  ServerInfoSnapShoot();
  ServerInfoSnapShoot(common::time64_t msec, IServerInfoSPtr info);
  bool isValid() const;

  common::time64_t msec;
  IServerInfoSPtr info;
};

class IDataBaseInfo {
 public:
  typedef std::vector<NDbKValue> keys_container_t;
  connectionTypes type() const;
  std::string name() const;
  size_t sizeDB() const;
  void setSizeDB(size_t size);
  size_t loadedSize() const;

  bool isDefault() const;
  void setIsDefault(bool isDef);

  virtual IDataBaseInfo* clone() const = 0;
  virtual ~IDataBaseInfo();

  keys_container_t keys() const;
  void setKeys(const keys_container_t& keys);
  void clearKeys();

 protected:
  IDataBaseInfo(const std::string& name, bool isDefault, connectionTypes type,
               size_t size, const keys_container_t& keys);

 private:
  const std::string name_;
  bool is_default_;
  size_t size_;
  keys_container_t keys_;

  const connectionTypes type_;
};

typedef common::shared_ptr<IDataBaseInfo> IDataBaseInfoSPtr;

}  // namespace core
}  // namespace fastonosql
