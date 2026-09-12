#include <QtTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <cstdio>
#include "commandline.h"
#include "logfile.h"
#ifdef Q_OS_WIN
#include <io.h>
#define LOG_DUP _dup
#define LOG_DUP2 _dup2
#define LOG_CLOSE _close
#define LOG_FILENO _fileno
#else
#include <unistd.h>
#define LOG_DUP dup
#define LOG_DUP2 dup2
#define LOG_CLOSE close
#define LOG_FILENO fileno
#endif

class StderrCapture {
public:
  StderrCapture() {
    std::fflush(stderr);
    if (file.open()) {
      saved = LOG_DUP(LOG_FILENO(stderr));
      if (saved >= 0) active = LOG_DUP2(file.handle(), LOG_FILENO(stderr)) >= 0;
    }
  }
  ~StderrCapture() { restore(); }
  QByteArray finish() { restore(); file.seek(0); return file.readAll(); }
  bool active = false;
private:
  void restore() {
    if (saved < 0) return;
    std::fflush(stderr);
    LOG_DUP2(saved, LOG_FILENO(stderr));
    LOG_CLOSE(saved);
    saved = -1;
  }
  int saved = -1;
  QTemporaryFile file;
};

class LoggingTest : public QObject
{
  Q_OBJECT
private slots:
  void arguments()
  {
    const auto parsed = CommandLine::parse({"reader", "--debug", "--show", "feed:https://example.org/--exit"});
    QVERIFY(parsed.debug);
    QVERIFY(parsed.error.isEmpty());
    QCOMPARE(parsed.messages, QStringList({"--show", "feed:https://example.org/--exit"}));
    QVERIFY(CommandLine::parse({"reader", "-h"}).help);
    QVERIFY(CommandLine::parse({"reader", "--version"}).version);
    QVERIFY(!CommandLine::parse({"reader", "--unknown"}).error.isEmpty());
    QVERIFY(!CommandLine::parse({"reader", "--", "--debug"}).debug);
    QVERIFY(!CommandLine::parse({"reader", "feed:https://example.org/\n--exit"}).error.isEmpty());
  }
  void loggingSinks()
  {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("debug.log");
    LogFile::configure(path, true, true);
    QByteArray console;
    {
      StderrCapture capture;
      QVERIFY(capture.active);
      LogFile::msgHandler(QtInfoMsg, {}, "ordinary info");
      LogFile::msgHandler(QtDebugMsg, {}, "suppressed debug");
      console = capture.finish();
    }
    QVERIFY(console.isEmpty());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray original = file.readAll();
    QVERIFY(original.contains("ordinary info"));
    QVERIFY(!original.contains("suppressed debug"));
    file.close();
    LogFile::setFileLoggingEnabled(false);
    LogFile::enableConsole();
    {
      StderrCapture capture;
      QVERIFY(capture.active);
      LogFile::msgHandler(QtDebugMsg, {}, "console debug");
      LogFile::msgHandler(QtWarningMsg, {}, "console warning");
      console = capture.finish();
    }
    QVERIFY(console.contains("console debug"));
    QVERIFY(console.contains("console warning"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), original);
    file.close();
    LogFile::setFileLoggingEnabled(true);
    {
      StderrCapture capture;
      QVERIFY(capture.active);
      LogFile::msgHandler(QtDebugMsg, {}, "both sinks");
      QVERIFY(capture.finish().contains("both sinks"));
    }
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.readAll().contains("both sinks"));
    LogFile::configure(QString(), false, true);
  }
};
QTEST_GUILESS_MAIN(LoggingTest)
#include "test_logging.moc"
