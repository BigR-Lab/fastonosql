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

#include "core/types.h"

#include "fasto/qt/gui/base/table_model.h"

namespace fastonosql {
class KeyTableItem
  : public fasto::qt::gui::TableItem {
 public:
  enum eColumn
  {
    kKey = 0,
    kType = 1,
    kTTL = 2,
    kCountColumns = 3
  };

  explicit KeyTableItem(const NDbKValue& key);

  QString key() const;
  QString typeText() const;
  int32_t TTL() const;
  common::Value::Type type() const;

  NDbKValue dbv() const;
  void setDbv(const NDbKValue& val);

 private:
  NDbKValue key_;
};

class KeysTableModel
        : public fasto::qt::gui::TableModel {
  Q_OBJECT
 public:
  explicit KeysTableModel(QObject *parent = 0);
  virtual ~KeysTableModel();

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role);
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex& parent) const;
  void clear();

  void changeValue(const NDbKValue& value);

 Q_SIGNALS:
  void changedValue(CommandKeySPtr cmd);
};

}


