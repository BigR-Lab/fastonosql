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

#include "core/database/idatabase.h"

#include <memory>  // for __shared_ptr
#include <string>  // for string

#include <common/macros.h>  // for DCHECK_EQ, CHECK

#include "core/events/events_info.h"  // for ClearDatabaseRequest, etc
#include "core/server/iserver.h"      // for IServer

namespace fastonosql {
namespace core {

IDatabase::IDatabase(IServerSPtr server, IDataBaseInfoSPtr info) : info_(info), server_(server) {
  CHECK(server);
  CHECK(info);
  CHECK(server->Type() == info->Type());
}

IDatabase::~IDatabase() {}

connectionTypes IDatabase::Type() const {
  return info_->Type();
}

IServerSPtr IDatabase::Server() const {
  return server_;
}

bool IDatabase::IsDefault() const {
  return info_->IsDefault();
}

std::string IDatabase::Name() const {
  return info_->Name();
}

void IDatabase::LoadContent(const events_info::LoadDatabaseContentRequest& req) {
  DCHECK_EQ(req.inf, info_);

  server_->LoadDatabaseContent(req);
}

void IDatabase::SetDefault(const events_info::SetDefaultDatabaseRequest& req) {
  DCHECK_EQ(req.inf, info_);

  server_->SetDefaultDB(req);
}

IDataBaseInfoSPtr IDatabase::Info() const {
  return info_;
}

translator_t IDatabase::Translator() const {
  return server_->Translator();
}

void IDatabase::Execute(const events_info::ExecuteInfoRequest& req) {
  server_->Execute(req);
}

void IDatabase::RemoveAllKeys(const events_info::ClearDatabaseRequest& req) {
  DCHECK_EQ(req.inf, info_);

  server_->ClearDB(req);
}

}  // namespace core
}  // namespace fastonosql
