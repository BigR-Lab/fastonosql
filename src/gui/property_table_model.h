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

#include "fasto/qt/gui/base/table_model.h"

#include "core/types.h"

namespace fastonosql {

struct PropertyTableItem
  : public fasto::qt::gui::TableItem {
  enum eColumn {
    eKey = 0,
    eValue = 1,
    eCountColumns = 2
  };
  PropertyTableItem(const QString& key, const QString& value);

  QString key_;
  QString value_;
};

class PropertyTableModel
  : public fasto::qt::gui::TableModel {
  Q_OBJECT
 public:
  explicit PropertyTableModel(QObject* parent = 0);
  ~PropertyTableModel();

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

  void changeProperty(const PropertyType& pr);

 Q_SIGNALS:
  void changedProperty(const PropertyType& pr);
};

}  // namespace fastonosql


