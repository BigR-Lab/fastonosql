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

#include "core/events/events_info.h"

namespace fastonosql {
namespace events {

typedef common::utils_qt::Event<EventsInfo::ConnectInfoRequest, QEvent::User + 1> ConnectRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ConnectInfoResponce, QEvent::User + 2> ConnectResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ShutDownInfoRequest, QEvent::User + 3> ShutDownRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ShutDownInfoResponce, QEvent::User + 4> ShutDownResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ProcessConfigArgsInfoRequest, QEvent::User + 5> ProcessConfigArgsRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ProcessConfigArgsInfoResponce, QEvent::User + 6> ProcessConfigArgsResponceEvent;

typedef common::utils_qt::Event<EventsInfo::CommandRootCreatedInfo, QEvent::User + 7> CommandRootCreatedEvent;
typedef common::utils_qt::Event<EventsInfo::CommandRootCompleatedInfo, QEvent::User + 8> CommandRootCompleatedEvent;

typedef common::utils_qt::Event<EventsInfo::EnterModeInfo, QEvent::User + 9> EnterModeEvent;
typedef common::utils_qt::Event<EventsInfo::LeaveModeInfo, QEvent::User + 10> LeaveModeEvent;

typedef common::utils_qt::Event<EventsInfo::DisConnectInfoRequest, QEvent::User + 11> DisconnectRequestEvent;
typedef common::utils_qt::Event<EventsInfo::DisConnectInfoResponce, QEvent::User + 12> DisconnectResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ExecuteInfoRequest, QEvent::User + 13> ExecuteRequestEvent;

typedef common::utils_qt::Event<EventsInfo::LoadDatabasesInfoRequest, QEvent::User + 15> LoadDatabasesInfoRequestEvent;
typedef common::utils_qt::Event<EventsInfo::LoadDatabasesInfoResponce, QEvent::User + 16> LoadDatabasesInfoResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ServerInfoRequest, QEvent::User + 17> ServerInfoRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ServerInfoResponce, QEvent::User + 18> ServerInfoResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ServerInfoHistoryRequest, QEvent::User + 19> ServerInfoHistoryRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ServerInfoHistoryResponce, QEvent::User + 20> ServerInfoHistoryResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ClearServerHistoryRequest, QEvent::User + 21> ClearServerHistoryRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ClearServerHistoryResponce, QEvent::User + 22> ClearServerHistoryResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ServerPropertyInfoRequest, QEvent::User + 23> ServerPropertyInfoRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ServerPropertyInfoResponce, QEvent::User + 24> ServerPropertyInfoResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ChangeServerPropertyInfoRequest, QEvent::User + 25> ChangeServerPropertyInfoRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ChangeServerPropertyInfoResponce, QEvent::User + 26> ChangeServerPropertyInfoResponceEvent;

typedef common::utils_qt::Event<EventsInfo::BackupInfoRequest, QEvent::User + 27> BackupRequestEvent;
typedef common::utils_qt::Event<EventsInfo::BackupInfoResponce, QEvent::User + 28> BackupResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ExportInfoRequest, QEvent::User + 29> ExportRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ExportInfoResponce, QEvent::User + 30> ExportResponceEvent;

typedef common::utils_qt::Event<EventsInfo::LoadDatabaseContentRequest, QEvent::User + 31> LoadDatabaseContentRequestEvent;
typedef common::utils_qt::Event<EventsInfo::LoadDatabaseContentResponce, QEvent::User + 32> LoadDatabaseContentResponceEvent;

typedef common::utils_qt::Event<EventsInfo::SetDefaultDatabaseRequest, QEvent::User + 33> SetDefaultDatabaseRequestEvent;
typedef common::utils_qt::Event<EventsInfo::SetDefaultDatabaseResponce, QEvent::User + 34> SetDefaultDatabaseResponceEvent;

typedef common::utils_qt::Event<EventsInfo::CommandRequest, QEvent::User + 35> CommandRequestEvent;
typedef common::utils_qt::Event<EventsInfo::CommandResponce, QEvent::User + 36> CommandResponceEvent;

typedef common::utils_qt::Event<EventsInfo::DiscoveryInfoRequest, QEvent::User + 37> DiscoveryInfoRequestEvent;
typedef common::utils_qt::Event<EventsInfo::DiscoveryInfoResponce, QEvent::User + 38> DiscoveryInfoResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ChangePasswordRequest, QEvent::User + 39> ChangePasswordRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ChangePasswordResponce, QEvent::User + 40> ChangePasswordResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ChangeMaxConnectionRequest, QEvent::User + 41> ChangeMaxConnectionRequestEvent;
typedef common::utils_qt::Event<EventsInfo::ChangeMaxConnectionResponce, QEvent::User + 42> ChangeMaxConnectionResponceEvent;

typedef common::utils_qt::Event<EventsInfo::ProgressInfoResponce, QEvent::User + 100> ProgressResponceEvent;

}
}
