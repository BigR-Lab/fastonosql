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

#include <QIcon>
#include <QFont>

#include "core/connection_types.h"

#include "common/value.h"
#include "common/patterns/singleton_pattern.h"

namespace fastonosql {

class GuiFactory
        : public common::patterns::LazySingleton<GuiFactory> {
 public:
  friend class common::patterns::LazySingleton<GuiFactory>;

  const QIcon& homePageIcon() const;
  const QIcon& facebookIcon() const;
  const QIcon& twitterIcon() const;
  const QIcon& githubIcon() const;

  const QIcon& openIcon() const;
  const QIcon& logoIcon() const;
  const QIcon& mainWindowIcon() const;
  const QIcon& connectIcon() const;
  const QIcon& disConnectIcon() const;
  const QIcon& serverIcon() const;
  const QIcon& addIcon() const;
  const QIcon& removeIcon() const;
  const QIcon& editIcon() const;
  const QIcon& messageBoxInformationIcon() const;
  const QIcon& messageBoxQuestionIcon() const;
  const QIcon& executeIcon() const;
  const QIcon& timeIcon() const;
  const QIcon& stopIcon() const;
  const QIcon& databaseIcon() const;
  const QIcon& keyIcon() const;

  const QIcon& icon(connectionTypes type) const;
  const QIcon& modeIcon(ConnectionMode mode) const;
  const QIcon& icon(common::Value::Type type) const;

  const QIcon& importIcon() const;
  const QIcon& exportIcon() const;

  const QIcon& loadIcon() const;
  const QIcon& clusterIcon() const;
  const QIcon& saveIcon() const;
  const QIcon& saveAsIcon() const;
  const QIcon& textIcon() const;
  const QIcon& tableIcon() const;
  const QIcon& treeIcon() const;
  const QIcon& loggingIcon() const;
  const QIcon& discoveryIcon() const;
  const QIcon& commandIcon() const;
  const QIcon& encodeDecodeIcon() const;
  const QIcon& preferencesIcon() const;

  const QIcon& leftIcon() const;
  const QIcon& rightIcon() const;

  const QIcon& close16Icon() const;
  const QIcon& commandIcon(connectionTypes type) const;

  const QIcon& successIcon() const;
  const QIcon& failIcon() const;
  const QIcon& unknownIcon() const;

  QFont font() const;
  const QString& pathToLoadingGif() const;

 private:
  const QIcon& redisConnectionIcon() const;
  const QIcon& memcachedConnectionIcon() const;
  const QIcon& ssdbConnectionIcon() const;
  const QIcon& leveldbConnectionIcon() const;
  const QIcon& rocksdbConnectionIcon() const;
  const QIcon& unqliteConnectionIcon() const;
  const QIcon& lmdbConnectionIcon() const;
};

}  // namespace fastonosql
