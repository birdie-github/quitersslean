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

#include "globals.h"
#include "settings.h"

LogFile::LogFile()
{
}

void LogFile::msgHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
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

  file.open(openMode);

  QTextStream stream;
  stream.setDevice(&file);
  stream.setCodec("UTF-8");

  if (file.isOpen()) {
    QString currentDateTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");
    switch (type) {
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
