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

#include "shell/rocksdb_lexer.h"

#include <vector>
#include <algorithm>

#include "core/rocksdb/rocksdb_driver.h"

namespace {
  const QString help = "help";
}

namespace fastonosql {

RocksdbApi::RocksdbApi(QsciLexer *lexer)
  : BaseQsciApi(lexer) {
}

void RocksdbApi::updateAutoCompletionList(const QStringList& context, QStringList& list) {
  for (QStringList::const_iterator it = context.begin(); it != context.end(); ++it) {
    QString val = *it;
    for (size_t i = 0; i < SIZEOFMASS(rocksdbCommands); ++i) {
      CommandInfo cmd = rocksdbCommands[i];
      if (canSkipCommand(cmd)) {
        continue;
      }

      QString jval = common::convertFromString<QString>(cmd.name);
      if (jval.startsWith(val, Qt::CaseInsensitive)) {
        list.append(jval + "?1");
      }
    }

    if (help.startsWith(val, Qt::CaseInsensitive)) {
      list.append(help + "?2");
    }
  }
}

QStringList RocksdbApi::callTips(const QStringList& context, int commas,
                                 QsciScintilla::CallTipsStyle style, QList<int>& shifts) {
  for (QStringList::const_iterator it = context.begin(); it != context.end() - 1; ++it) {
    QString val = *it;
    for (size_t i = 0; i < SIZEOFMASS(rocksdbCommands); ++i) {
      CommandInfo cmd = rocksdbCommands[i];

      QString jval = common::convertFromString<QString>(cmd.name);
      if (QString::compare(jval, val, Qt::CaseInsensitive) == 0) {
        return QStringList() << makeCallTip(cmd);
      }
    }
  }

  return QStringList();
}

RocksdbLexer::RocksdbLexer(QObject* parent)
  : BaseQsciLexer(parent) {
  setAPIs(new RocksdbApi(this));
}

const char *RocksdbLexer::language() const {
  return "Rocksdb";
}

const char* RocksdbLexer::version() const {
  return RocksdbDriver::versionApi();
}

const char* RocksdbLexer::basedOn() const {
  return "rocksdb";
}

std::vector<uint32_t> RocksdbLexer::supportedVersions() const {
  std::vector<uint32_t> result;
  for (int i = 0; i < SIZEOFMASS(rocksdbCommands); ++i) {
    CommandInfo cmd = rocksdbCommands[i];

    bool needed_insert = true;
    for (size_t j = 0; j < result.size(); ++j) {
      if (result[j] == cmd.since) {
        needed_insert = false;
        break;
      }
    }

    if (needed_insert) {
      result.push_back(cmd.since);
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

uint32_t RocksdbLexer::commandsCount() const {
  return SIZEOFMASS(rocksdbCommands);
}

void RocksdbLexer::styleText(int start, int end) {
  if (!editor()) {
    return;
  }

  char *data = new char[end - start + 1];
  editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
  QString source(data);
  delete [] data;

  if (source.isEmpty()) {
    return;
  }

  paintCommands(source, start);

  int index = 0;
  int begin = 0;
  while ((begin = source.indexOf(help, index, Qt::CaseInsensitive)) != -1) {
    index = begin + help.length();

    startStyling(start + begin);
    setStyling(help.length(), HelpKeyword);
    startStyling(start + begin);
  }
}

void RocksdbLexer::paintCommands(const QString& source, int start) {
  for (size_t i = 0; i < SIZEOFMASS(rocksdbCommands); ++i) {
    CommandInfo cmd = rocksdbCommands[i];
    QString word = common::convertFromString<QString>(cmd.name);
    int index = 0;
    int begin = 0;
    while ((begin = source.indexOf(word, index, Qt::CaseInsensitive)) != -1) {
      index = begin + word.length();

      startStyling(start + begin);
      setStyling(word.length(), Command);
      startStyling(start + begin);
    }
  }
}

}  // namespace fastonosql
