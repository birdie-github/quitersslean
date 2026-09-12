// SPDX-License-Identifier: GPL-3.0-or-later
#include "filemanager.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#include <QDebug>
#ifdef HAVE_FILEMANAGER_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

void FileManager::openDirectory(const QString &directory)
{
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory)))
    qWarning() << "Could not open directory:" << directory;
}

void FileManager::showFile(const QString &fileName)
{
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
      if (reply.isError()) FileManager::openDirectory(directory);
      finished->deleteLater();
    });
    return;
#endif
  }
  FileManager::openDirectory(directory);
}
