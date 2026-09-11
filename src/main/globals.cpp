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
#include "projectmetadata.h"
#include "globals.h"

#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringBuilder>

#include "settings.h"

Globals globals;

Globals::Globals()
  : logFileOutput_(true)
  , noDebugOutput_(true)
  , isInit_(false)
  , isPortable_(false)
  , resourcesDir_()
  , dataDir_()
  , cacheDir_()
  , soundNotifyDir_()
{

}

void Globals::init()
{
  // isPortable ...
#if defined(Q_OS_WIN)
  isPortable_ = true;
  QString fileName(QCoreApplication::applicationDirPath() + ("/" + ProjectMetadata::portableMarker()));
  if (!QFile::exists(fileName)) {
    isPortable_ = false;
  }
#endif

  // Check Dir ...
#if defined(Q_OS_WIN)
  resourcesDir_ = QCoreApplication::applicationDirPath();
#else
#if defined(Q_OS_MAC)
  resourcesDir_ = QCoreApplication::applicationDirPath() + "/../Resources";
#else
  resourcesDir_ = RESOURCES_DIR;
#endif
#endif

  if (isPortable_) {
    dataDir_ = QCoreApplication::applicationDirPath();
    cacheDir_ = QDir(dataDir_).filePath(ProjectMetadata::cache());
    soundNotifyDir_ = QDir(dataDir_).filePath(ProjectMetadata::sounds());
  } else {
    dataDir_ = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)).filePath(QCoreApplication::applicationName());
    cacheDir_ = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)).filePath(QCoreApplication::applicationName());
    soundNotifyDir_ = QDir(resourcesDir_).filePath(ProjectMetadata::sounds());

    QDir dir(dataDir_);
    dir.mkpath(dataDir_);
  }

  // settings ...
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QString settingsFileName;
  const QString configDirectory = isPortable_ ? dataDir_ :
      QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
          .filePath(QCoreApplication::applicationName());
  QDir().mkpath(configDirectory);
  settingsFileName = QDir(configDirectory).filePath(QCoreApplication::applicationName() + ".ini");
  Settings::createSettings(settingsFileName);

  Settings settings("Settings");
  noDebugOutput_ = settings.value("noDebugOutput", true).toBool();
  userAgent_ = settings.value("userAgent", "Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/77.0.3865.120 Safari/537.36").toString();

  isInit_ = true;
}
