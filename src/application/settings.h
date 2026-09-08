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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QStringList>
#include <QVariant>

class Settings
{
public:
  static void createSettings(const QString &fileName = QString());
  static void syncSettings();
  QString fileName();

  void beginGroup(const QString &prefix);
  void endGroup();

  void setValue(const QString &key, const QVariant &defaultValue = QVariant());
  QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
  bool contains(const QString &key);

private:
  static QSettings *storage();
  QString fullKey(const QString &key) const;
  QStringList groups_;

};

#endif // SETTINGS_H
