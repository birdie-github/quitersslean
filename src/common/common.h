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
#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QDir>

#ifndef QSL
#define QSL(x) QStringLiteral(x)
#endif

#ifndef QL1S
#define QL1S(x) QLatin1String(x)
#endif

#ifndef QL1C
#define QL1C(x) QLatin1Char(x)
#endif

namespace Common
{

  bool removePath(const QString &path);
  bool matchDomain(const QString &pattern, const QString &domain);
  QString filterCharsFromFilename(const QString &name);
  QString ensureUniqueFilename(const QString &name, const QString &appendFormat = QString("(%1)"));
  void createFileBackup(const QString &oldFilename, const QString &oldVersion);

  QString readAllFileContents(const QString &filename);
  QByteArray readAllFileByteContents(const QString &filename);

  void sleep(int ms);

  QString operatingSystem();
  QString cpuArchitecture();
  QString operatingSystemLong();
}

#endif // COMMON_H
