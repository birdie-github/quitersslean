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
#include "mainapplication.h"

#include "common.h"
#include "cookiejar.h"
#include "database.h"
#include "globals.h"
#include "networkmanager.h"
#include "settings.h"
#include "splashscreen.h"
#include "updatefeeds.h"
#include "VersionNo.h"

#include <QScreen>
#include <QDesktopServices>
#include <QProcess>
#include "articlecontent.h"

MainApplication::MainApplication(int &argc, char **argv)
  : QtSingleApplication(argc, argv)
  , isPortableAppsCom_(false)
  , isClosing_(false)
  , dbFileExists_(false)
  , translator_(0)
  , qt_translator_(0)
  , mainWindow_(0)
  , networkManager_(0)
  , cookieJar_(0)
  , diskCache_(0)
  , downloadManager_(0)
{
  setApplicationName(STRAPPLICATIONNAME);
  setOrganizationName(STRAPPLICATIONNAME);
  setApplicationVersion(STRPRODUCTVER);
  globals.init();

  QString message = arguments().value(1);
  if (isRunning()) {
    if (argc == 1) {
      sendMessage("--show");
    } else {
      for (int i = 2; i < argc; ++i)
        message += '\n' + arguments().value(i);
      sendMessage(message);
    }
    isClosing_ = true;
    return;
  } else {
    if (message.contains("--exit", Qt::CaseInsensitive)) {
      isClosing_ = true;
      return;
    }
  }

  setWindowIcon(QIcon(":/images/quiterss128"));
  setQuitOnLastWindowClosed(false);

  createSettings();

  qWarning() << "Run application!";

  setStyleApplication();
  setTranslateApplication();
  showSplashScreen();

  connectDatabase();
  setProgressSplashScreen(30);
  qWarning() << "Run application 2";
  mainWindow_ = new MainWindow();
  qWarning() << "Run application 3";
  setProgressSplashScreen(60);

  qWarning() << "Run application 4";
  updateFeeds_ = new UpdateFeeds(mainWindow_);
  setProgressSplashScreen(90);
  qWarning() << "Run application 5";
  mainWindow_->restoreFeedsOnStartUp();
  setProgressSplashScreen(100);
  qWarning() << "Run application 6";
  if (!mainWindow_->startingTray_ || !mainWindow_->showTrayIcon_) {
    mainWindow_->show();
  }
  mainWindow_->isMinimizeToTray_ = false;

  closeSplashScreen();

  if (mainWindow_->showTrayIcon_) {
    QTimer::singleShot(0, mainWindow_->trayIcon(), SLOT(show()));
  }

  if (updateFeedsStartUp_) {
    QTimer::singleShot(0, mainWindow_, SLOT(slotGetAllFeeds()));
  }

  receiveMessage(message);
  connect(this, SIGNAL(messageReceived(QString)), SLOT(receiveMessage(QString)));
}

MainApplication::~MainApplication()
{

}

MainApplication *MainApplication::getInstance()
{
  return static_cast<MainApplication*>(QCoreApplication::instance());
}

void MainApplication::receiveMessage(const QString &message)
{
  if (!message.isEmpty()) {
    qWarning() << QString("Received message: %1").arg(message);

    QStringList params = message.split('\n');
    foreach (QString param, params) {
      if (param == "--show") {
        if (isClosing_)
          return;
        mainWindow_->showWindows();
      }
      if (param == "--exit") mainWindow_->quitApp();
      if (param.contains("feed:", Qt::CaseInsensitive)) {
        QClipboard *clipboard = QApplication::clipboard();
        if (param.contains("https://", Qt::CaseInsensitive)) {
          param.remove(0, 5);
          clipboard->setText(param);
        } else {
          param.remove(0, 7);
          clipboard->setText("http://" + param);
        }
        mainWindow_->addFeed();
      }
    }
  }
}

void MainApplication::createSettings()
{
  Settings settings("Settings");
  storeDBMemory_ = AppSettings::storeDBMemory.get();
  isSaveDataLastFeed_ = settings.value("createLastFeed", false).toBool();
  styleApplication_ = settings.value("styleApplication", "greenStyle_").toString();
  showSplashScreen_ = AppSettings::showSplashScreen.get();
  updateFeedsStartUp_ = AppSettings::autoUpdatefeedsStartUp.get();

  QString strLang;
  QString strLocalLang = QLocale::system().name();
  bool findLang = false;
  QDir langDir(resourcesDir() + "/lang");
  foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
    strLang = file.section('.', 0, 0).section('_', 1);
    if (strLocalLang == strLang) {
      strLang = strLocalLang;
      findLang = true;
      break;
    }
  }
  if (!findLang) {
    strLocalLang = strLocalLang.left(2);
    foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
      strLang = file.section('.', 0, 0).section('_', 1);
      if (strLocalLang.contains(strLang, Qt::CaseInsensitive)) {
        strLang = strLocalLang;
        findLang = true;
        break;
      }
    }
  }
  if (!findLang) strLang = "en";
  langFileName_ = settings.value("langFileName", strLang).toString();

  proxyLoadSettings();
}

void MainApplication::connectDatabase()
{
  QString fileName(dbFileName() % ".bak");
  if (QFile(fileName).exists()) {
    QString sourceFileName = QFile::symLinkTarget(dbFileName());
    if (sourceFileName.isEmpty()) {
      sourceFileName = dbFileName();
    }
    if (QFile::remove(sourceFileName)) {
      if (!QFile::rename(fileName, sourceFileName))
        qCritical() << "Failed to rename new database file!";
    } else {
      qCritical() << "Failed to delete old database file!";
    }
  }

  if (QFile(dbFileName()).exists()) {
    dbFileExists_ = true;
  }

  Database::initialization();
}

void MainApplication::quitApplication()
{
  qWarning() << "quitApplication 1";
  delete mainWindow_;
  qWarning() << "quitApplication 2";
  delete networkManager_;
  delete cookieJar_;
  delete closingWidget_;

  qWarning() << "Quit application";

  quit();
}

void MainApplication::showClosingWidget()
{
  closingWidget_ = new QWidget(0, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
  closingWidget_->setFocusPolicy(Qt::NoFocus);
  QVBoxLayout *layout = new QVBoxLayout(closingWidget_);
  layout->addWidget(new QLabel(tr("Saving data...")));
  closingWidget_->resize(150, 20);
  closingWidget_->show();
  if (QScreen *screen = QGuiApplication::primaryScreen()) {
    const QRect available = screen->availableGeometry();
    closingWidget_->move(available.x() + available.width() - closingWidget_->frameSize().width(),
                         available.y() + available.height() - closingWidget_->frameSize().height());
  }
  closingWidget_->setFixedSize(closingWidget_->size());
  qApp->processEvents();
}

void MainApplication::commitData(QSessionManager &manager)
{
  manager.release();
  mainWindow_->quitApp();
}

bool MainApplication::isPortable() const
{
  return globals.isPortable_;
}

bool MainApplication::isPortableAppsCom() const
{
  return isPortableAppsCom_;
}

void MainApplication::setClosing()
{
  isClosing_ = true;
}

bool MainApplication::isClosing() const
{
  return isClosing_;
}

bool MainApplication::isNoDebugOutput() const
{
  return globals.noDebugOutput_;
}

QString MainApplication::resourcesDir() const
{
  return globals.resourcesDir_;
}

QString MainApplication::dataDir() const
{
  return globals.dataDir_;
}

QString MainApplication::absolutePath(const QString &path) const
{
  QString absolutePath = path;
  if (isPortable()) {
    if (!QDir::isAbsolutePath(path)) {
      absolutePath = dataDir() % "/" % path;
    }
  }
  return absolutePath;
}

QString MainApplication::dbFileName() const
{
  return dataDir() % "/feeds.db";
}

bool MainApplication::isSaveDataLastFeed() const
{
  return isSaveDataLastFeed_;
}

bool MainApplication::storeDBMemory() const
{
  return storeDBMemory_;
}

void MainApplication::setStyleApplication()
{
  QString fileName(resourcesDir());
  if (styleApplication_ == "systemStyle_") {
    fileName.append("/style/system.qss");
  } else if (styleApplication_ == "system2Style_") {
    fileName.append("/style/system2.qss");
  } else if (styleApplication_ == "darkStyle_") {
    fileName.append("/style/dark.qss");
  } else if (styleApplication_ == "orangeStyle_") {
    fileName.append("/style/orange.qss");
  } else if (styleApplication_ == "purpleStyle_") {
    fileName.append("/style/purple.qss");
  } else if (styleApplication_ == "pinkStyle_") {
    fileName.append("/style/pink.qss");
  } else if (styleApplication_ == "grayStyle_") {
    fileName.append("/style/gray.qss");
  } else {
    fileName.append("/style/green.qss");
  }
  QFile file(fileName);
  const QByteArray styleData = file.open(QFile::ReadOnly)
      ? file.readAll() : Common::readAllFileByteContents(":/style/systemStyle");
  setStyleSheet(QLatin1String(styleData));
  file.close();

  setStyle(new QProxyStyle);
}

void MainApplication::setTranslateApplication()
{
  if (!translator_)
    translator_ = new QTranslator(this);
  removeTranslator(translator_);
  if (translator_->load(resourcesDir() + QString("/lang/quiterss_%1").arg(langFileName_)))
    installTranslator(translator_);

  if (!qt_translator_)
    qt_translator_ = new QTranslator(this);
  removeTranslator(qt_translator_);
#ifdef HAVE_X11
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const QString translationsDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
  const QString translationsDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
#else
  const QString translationsDir = resourcesDir() + "/lang";
#endif
  if (qt_translator_->load(translationsDir + "/qtbase_" + langFileName_))
    installTranslator(qt_translator_);
}

void MainApplication::showSplashScreen()
{
  Settings settings;
  int versionDB = settings.value("VersionDB", "1").toInt();
  if ((versionDB != Database::version()) && QFile::exists(settings.fileName()))
    showSplashScreen_ = true;

  if (showSplashScreen_) {
    splashScreen_ = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
    splashScreen_->show();
    processEvents();
    if ((versionDB != Database::version()) && QFile::exists(settings.fileName())) {
      splashScreen_->showMessage(QString("Converting database to version %1...").arg(Database::version()),
                                Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    }
  }
}

void MainApplication::closeSplashScreen()
{
  if (showSplashScreen_) {
    splashScreen_->finish(mainWindow_);
    splashScreen_->deleteLater();
  }
}

void MainApplication::setProgressSplashScreen(int value)
{
  if (showSplashScreen_)
    splashScreen_->setProgress(value);
}

MainWindow *MainApplication::mainWindow()
{
  return mainWindow_;
}

NetworkManager *MainApplication::networkManager()
{
  if (!networkManager_) {
    networkManager_ = new NetworkManager(false, this);
    setDiskCache();
  }
  return networkManager_;
}

CookieJar *MainApplication::cookieJar()
{
  if (!cookieJar_) {
    cookieJar_ = new CookieJar(this);
  }
  return cookieJar_;
}

void MainApplication::setDiskCache()
{
  Settings settings("Settings");

  bool useDiskCache = settings.value("useDiskCache", true).toBool();
  if (useDiskCache) {
    if (!diskCache_) {
      diskCache_ = new QNetworkDiskCache(this);
    }

    QString diskCacheDirPath = settings.value("dirDiskCache", cacheDefaultDir()).toString();
    if (diskCacheDirPath.isEmpty()) diskCacheDirPath = cacheDefaultDir();
    diskCacheDirPath = absolutePath(diskCacheDirPath);

    bool cleanDiskCache = settings.value("cleanDiskCache", true).toBool();
    if (cleanDiskCache) {
      Common::removePath(diskCacheDirPath);
      settings.setValue("cleanDiskCache", false);
    }

    diskCache_->setCacheDirectory(diskCacheDirPath);
    int maxDiskCache = settings.value("maxDiskCache", 50).toInt();
    diskCache_->setMaximumCacheSize(maxDiskCache*1024*1024);

    networkManager()->setCache(diskCache_);
  } else {
    if (diskCache_) {
      diskCache_->setMaximumCacheSize(0);
      diskCache_->clear();
    }
  }
}

QString MainApplication::cacheDefaultDir() const
{
  return globals.cacheDir_;
}

QString MainApplication::soundNotifyDefaultFile() const
{
  return globals.soundNotifyDir_ % "/notification.wav";
}

QString MainApplication::styleSheetNewsDefaultFile() const
{
  if (isPortable()) {
    return "style/news.css";
  } else {
    return resourcesDir() % "/style/news.css";
  }
}

UpdateFeeds *MainApplication::updateFeeds()
{
  return updateFeeds_;
}

void MainApplication::runUserFilter(int feedId, int filterId)
{
  emit signalRunUserFilter(feedId, filterId);
}

void MainApplication::sqlQueryExec(const QString &query)
{
  emit signalSqlQueryExec(query);
}

DownloadManager *MainApplication::downloadManager()
{
  if (!downloadManager_) {
    downloadManager_ = new DownloadManager();
  }
  return downloadManager_;
}

void MainApplication::proxyLoadSettings()
{
  Settings settings("networkProxy");
  networkProxy_.setType(static_cast<QNetworkProxy::ProxyType>(
                          settings.value("type", QNetworkProxy::DefaultProxy).toInt()));
  networkProxy_.setHostName(settings.value("hostName", "").toString());
  networkProxy_.setPort(    settings.value("port",     "").toUInt());
  networkProxy_.setUser(    settings.value("user",     "").toString());
  networkProxy_.setPassword(settings.value("password", "").toString());

  setProxy();
}

void MainApplication::proxySaveSettings(const QNetworkProxy &proxy)
{
  networkProxy_ = proxy;

  Settings settings("networkProxy");
  settings.setValue("type",     networkProxy_.type());
  settings.setValue("hostName", networkProxy_.hostName());
  settings.setValue("port",     networkProxy_.port());
  settings.setValue("user",     networkProxy_.user());
  settings.setValue("password", networkProxy_.password());

  setProxy();
}

void MainApplication::setProxy()
{

  if (QNetworkProxy::DefaultProxy == networkProxy_.type())
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  else
    QNetworkProxy::setApplicationProxy(networkProxy_);
}

bool MainApplication::openExternalUrl(const QUrl &url)
{
  if (!ArticleContent::isExternalLink(url)) return false;
  const int mode = AppSettings::externalBrowserOn.get();
  if (url.scheme() == QLatin1String("mailto") || (mode != 2 && mode != -1)) {
    qInfo() << "Opening" << QString::fromUtf8(url.toEncoded())
            << (url.scheme() == QLatin1String("mailto")
                ? "using default email application (system URL handler)"
                : "using default web browser (system URL handler)");
    const bool opened = QDesktopServices::openUrl(url);
    if (!opened) qInfo() << "Failed to hand off" << QString::fromUtf8(url.toEncoded());
    return opened;
  }

  const QString command = AppSettings::externalBrowser.get().trimmed();
  const QString link = QString::fromUtf8(url.toEncoded());
  bool started = false;
  if (!command.isEmpty()) {
#ifdef Q_OS_MAC
    // Preserve the legacy macOS application-name/application-bundle behavior.
    const QStringList arguments = QStringList() << "-a" << command << link;
    qInfo() << "Launching" << QStringLiteral("open") << "arguments:" << arguments;
    started = QProcess::startDetached(QStringLiteral("open"), arguments);
#else
    // Preserve command arguments without passing article URLs through a shell.
    // Also accept an unquoted executable path containing spaces.
    QStringList arguments = QFileInfo(command).isFile()
        ? QStringList(command) : QProcess::splitCommand(command);
    if (!arguments.isEmpty()) {
      const QString program = arguments.takeFirst();
      arguments.append(link);
      qInfo() << "Launching" << program << "arguments:" << arguments;
      started = QProcess::startDetached(program, arguments);
    }
#endif
  }
  if (!started) {
    qInfo() << "Failed to launch custom browser" << command << "for" << link;
    QMessageBox::warning(mainWindow_, tr("External Browser"),
                         tr("Could not start the configured browser. Check the custom browser setting in Article View."));
  }
  return started;
}
