/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include "core/core_fwd.h"

#include "events/events_info.h"

namespace fastonosql {

class IDatabase {
 public:
  virtual ~IDatabase();

  connectionTypes type() const;
  IServerSPtr server() const;
  bool isDefault() const;
  std::string name() const;

  void loadContent(const events_info::LoadDatabaseContentRequest &req);
  void setDefault(const events_info::SetDefaultDatabaseRequest &req);

  IDataBaseInfoSPtr info() const;
  void setInfo(IDataBaseInfoSPtr info);

  void executeCommand(const events_info::CommandRequest& req);

 protected:
  IDatabase(IServerSPtr server, IDataBaseInfoSPtr info);

  IDataBaseInfoSPtr info_;
  const IServerSPtr server_;
};

}  // namespace fastonosql
