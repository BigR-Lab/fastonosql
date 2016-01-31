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

#include <QStringList>

#include "global/types.h"

#include "common/patterns/singleton_pattern.h"

#include "core/connection_settings.h"

namespace fastonosql {

class SettingsManager
  : public common::patterns::LazySingleton<SettingsManager> {
 public:
  typedef std::vector<IConnectionSettingsBaseSPtr> ConnectionSettingsContainerType;
  typedef std::vector<IClusterSettingsBaseSPtr> ClusterSettingsContainerType;
  friend class common::patterns::LazySingleton<SettingsManager>;

  static QString settingsDirPath();
  static std::string settingsFilePath();

  void setDefaultView(supportedViews view);
  supportedViews defaultView() const;

  QString currentStyle() const;
  void setCurrentStyle(const QString &style);

  QString currentFontName() const;
  void setCurrentFontName(const QString& font);

  QString currentLanguage() const;
  void setCurrentLanguage(const QString &lang);

  // connections
  void addConnection(IConnectionSettingsBaseSPtr connection);
  void removeConnection(IConnectionSettingsBaseSPtr connection);

  ConnectionSettingsContainerType connections() const;

  // clusters
  void addCluster(IClusterSettingsBaseSPtr cluster);
  void removeCluster(IClusterSettingsBaseSPtr cluster);

  ClusterSettingsContainerType clusters() const;

  void addRConnection(const QString& connection);
  void removeRConnection(const QString& connection);
  QStringList recentConnections() const;
  void clearRConnections();

  bool syncTabs() const;
  void setSyncTabs(bool sync);

  void setLoggingDirectory(const QString& dir);
  QString loggingDirectory() const;

  bool autoCheckUpdates() const;
  void setAutoCheckUpdates(bool isCheck);

  bool autoCompletion() const;
  void setAutoCompletion(bool enableAuto);

  bool autoOpenConsole() const;
  void setAutoOpenConsole(bool enableAuto);

  bool fastViewKeys() const;
  void setFastViewKeys(bool fastView);

  void reloadFromPath(const std::string& path, bool merge);

 private:
  void load();
  void save();

  SettingsManager();
  ~SettingsManager();

  supportedViews views_;
  QString curStyle_;
  QString curFontName_;
  QString curLanguage_;
  ConnectionSettingsContainerType connections_;
  ClusterSettingsContainerType clusters_;
  QStringList recentConnections_;
  bool syncTabs_;
  QString loggingDir_;
  bool autoCheckUpdate_;
  bool autoCompletion_;
  bool autoOpenConsole_;
  bool fastViewKeys_;
};

}
