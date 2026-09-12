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

#include <QDir>
#include <cstdio>
#include <QMutex>
#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#ifdef Q_OS_WIN
#include <windows.h>
#include <io.h>
#endif
#ifdef HAVE_FILEMANAGER_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif


LogFile::LogFile()
{
}

namespace {
struct LogState {
  QMutex mutex;
  QString fileName;
  bool fileEnabled = true;
  bool consoleEnabled = false;
  bool suppressDebug = true;
};

LogState &logState()
{
  // Qt can log during static destruction, after QApplication has gone away.
  static LogState *state = new LogState;
  return *state;
}

void openLogDirectory(const QString &directory)
{
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory)))
    qWarning() << "Could not open log directory:" << directory;
}
}

void LogFile::configure(const QString &fileName, bool enabled, bool suppressDebug)
{
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  state.fileName = fileName;
  state.fileEnabled = enabled;
  state.suppressDebug = suppressDebug;
}

void LogFile::prepareConsole()
{
#ifdef Q_OS_WIN
  // Preserve shell redirection. GUI-subsystem executables may have no CRT streams.
  const bool needsOut = _fileno(stdout) < 0 || _get_osfhandle(_fileno(stdout)) == -1;
  const bool needsErr = _fileno(stderr) < 0 || _get_osfhandle(_fileno(stderr)) == -1;
  if (!needsOut && !needsErr) return;
  if (!AttachConsole(ATTACH_PARENT_PROCESS) && !GetConsoleWindow()) AllocConsole();
  auto reopen = [](FILE *stream) {
#ifdef _MSC_VER
    FILE *result = nullptr;
    freopen_s(&result, "CONOUT$", "w", stream);
#else
    (void)std::freopen("CONOUT$", "w", stream);
#endif
  };
  if (needsOut) reopen(stdout);
  if (needsErr) reopen(stderr);
#endif
}

void LogFile::enableConsole()
{
  prepareConsole();
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  state.consoleEnabled = true;
}

bool LogFile::fileLoggingEnabled()
{
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  return state.fileEnabled;
}

bool LogFile::consoleLoggingEnabled()
{
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  return state.consoleEnabled;
}

void LogFile::setFileLoggingEnabled(bool enabled)
{
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  state.fileEnabled = enabled;
}

void LogFile::msgHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
  auto &state = logState();
  QMutexLocker lock(&state.mutex);
  if (!state.consoleEnabled && (msg.startsWith("libpng warning: iCCP") ||
      (type == QtDebugMsg && state.suppressDebug))) return;
  const char *level = "INFO";
  switch (type) {
  case QtDebugMsg: level = "DEBUG"; break;
  case QtWarningMsg: level = "WARNING"; break;
  case QtCriticalMsg: level = "CRITICAL"; break;
  case QtFatalMsg: level = "FATAL"; break;
  default: break;
  }
  const QByteArray line = (QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz") +
      " " + QLatin1String(level) + ": " + msg + '\n').toUtf8();
  if (state.consoleEnabled) {
    std::fwrite(line.constData(), 1, size_t(line.size()), stderr);
    std::fflush(stderr);
  }
  if (!state.fileEnabled || state.fileName.isEmpty()) return;
  QFile file(state.fileName);
  QIODevice::OpenMode mode = QIODevice::WriteOnly;
  if (file.exists() && file.size() < qint64(maxLogFileSize)) mode |= QIODevice::Append;
  if (file.open(mode)) {
    file.write(line);
    file.flush();
  }
  // Returning from a QtFatalMsg handler lets Qt perform its normal fatal abort.
}

void LogFile::showLocation()
{
  QString fileName;
  {
    auto &state = logState();
    QMutexLocker lock(&state.mutex);
    fileName = state.fileName;
  }
  if (fileName.isEmpty()) return;
  const QString directory = QFileInfo(fileName).absolutePath();
  if (QFileInfo::exists(fileName)) {
#ifdef Q_OS_WIN
    if (QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(fileName)})) return;
#elif defined(Q_OS_MAC)
    if (QProcess::startDetached("/usr/bin/open", {"-R", fileName})) return;
#elif defined(HAVE_FILEMANAGER_DBUS)
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.FileManager1",
        "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1", "ShowItems");
    message << QStringList{QUrl::fromLocalFile(fileName).toString(QUrl::FullyEncoded)} << QString();
    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(message, 1500), QCoreApplication::instance());
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                     [directory](QDBusPendingCallWatcher *finished) {
      const QDBusPendingReply<> reply = *finished;
      if (reply.isError()) openLogDirectory(directory);
      finished->deleteLater();
    });
    return;
#endif
  }
  openLogDirectory(directory);
}
