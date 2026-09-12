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
#include "globals.h"
#include "mainapplication.h"
#include "logfile.h"
#include "commandline.h"
#include "projectmetadata.h"
#include <cstdio>

int main(int argc, char **argv)
{
  QStringList arguments;
  for (int i = 0; i < argc; ++i) arguments.append(QString::fromLocal8Bit(argv[i]));
  const auto options = CommandLine::parse(arguments);
  if (options.help || options.version) {
    LogFile::prepareConsole();
    const QByteArray text = (options.help ? CommandLine::helpText() :
        ProjectMetadata::name() + " " + ProjectMetadata::version() + "\n").toUtf8();
    std::fwrite(text.constData(), 1, size_t(text.size()), stdout);
    return 0;
  }
  if (options.debug) LogFile::enableConsole();
  qInstallMessageHandler(LogFile::msgHandler);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

  MainApplication app(argc, argv);

  if (app.isClosing())
    return app.startupExitCode();

  return app.exec();
}
