/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* © 2011-2020 QuiteRSS Project
* © 2026 Artem S. Tashkinov <aros@gmx.com> and ChatGPT
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "logfile.h"

#include <QStandardPaths>
#include <QDir>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <cstdio>

#include "globals.h"
#include "settings.h"

LogFile::LogFile()
{
}

void LogFile::msgHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
  // Launch diagnostics must reach the console even when file logging is enabled.
  if (type == QtInfoMsg) {
    const QByteArray text = msg.toLocal8Bit();
    std::fprintf(stderr, "%s\n", text.constData());
    std::fflush(stderr);
  }
  if (!globals.isInit_)
    return;
  if (msg.startsWith("libpng warning: iCCP"))
    return;

  if (type == QtDebugMsg) {
    if (globals.noDebugOutput_)
      return;
  }

  QFile file;
  file.setFileName(globals.dataDir_ + "/debug.log");
  QIODevice::OpenMode openMode = QIODevice::WriteOnly | QIODevice::Text;

  if (file.exists() && (file.size() < (qint64)maxLogFileSize)) {
    openMode |= QIODevice::Append;
  }

  if (!file.open(openMode)) return;

  QTextStream stream;
  stream.setDevice(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  stream.setEncoding(QStringConverter::Utf8);
#else
  stream.setCodec("UTF-8");
#endif

  if (file.isOpen()) {
    QString currentDateTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");
    switch (type) {
    case QtInfoMsg:
      stream << currentDateTime << " INFO: " << msg << "\n";
      break;
    case QtDebugMsg:
      stream << currentDateTime << " DEBUG: " << msg << "\n";
      break;
    case QtWarningMsg:
      stream << currentDateTime << " WARNING: " << msg << "\n";
      break;
    case QtCriticalMsg:
      stream << currentDateTime << " CRITICAL: " << msg << "\n";
      break;
    case QtFatalMsg:
      stream << currentDateTime << " FATAL: " << msg << "\n";
      qApp->exit(EXIT_FAILURE);
    default:
      break;
    }

    stream.flush();
    file.flush();
    file.close();
  }
}
