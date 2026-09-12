#include "applicationstyle.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>

namespace {
// Catch truncated files without depending on Qt's private stylesheet parser.
// Qt itself reports unsupported properties and other grammar errors on apply.
bool balanced(const QString &sheet)
{
  int braces = 0;
  QChar quote;
  bool comment = false;
  for (int i = 0; i < sheet.size(); ++i) {
    const QChar c = sheet.at(i);
    const QChar next = i + 1 < sheet.size() ? sheet.at(i + 1) : QChar();
    if (comment) {
      if (c == '*' && next == '/') { comment = false; ++i; }
    } else if (!quote.isNull()) {
      if (c == '\\') ++i;
      else if (c == quote) quote = QChar();
    } else if (c == '/' && next == '*') {
      comment = true;
      ++i;
    } else if (c == '\'' || c == '"') {
      quote = c;
    } else if (c == '{') {
      ++braces;
    } else if (c == '}' && --braces < 0) {
      return false;
    }
  }
  return braces == 0 && quote.isNull() && !comment;
}

bool readStyle(const QFileInfo &info, ApplicationStyle &style)
{
  QFile file(info.absoluteFilePath());
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Application style: cannot open" << file.fileName() << file.errorString();
    return false;
  }
  QByteArray bytes = file.readAll();
  if (file.error() != QFileDevice::NoError) {
    qWarning() << "Application style: cannot read" << file.fileName() << file.errorString();
    return false;
  }
  if (bytes.startsWith("\xef\xbb\xbf")) bytes.remove(0, 3);
  style.sheet = QString::fromUtf8(bytes);
  if (style.sheet.toUtf8() != bytes) {
    qWarning() << "Application style: invalid UTF-8" << file.fileName();
    return false;
  }
  if (style.sheet.trimmed().isEmpty() || !balanced(style.sheet)) {
    qWarning() << "Application style: empty or unbalanced QSS" << file.fileName();
    return false;
  }
  style.id = info.completeBaseName();
  style.name = style.id;
  style.fileName = file.fileName();
  static const QRegularExpression header(
      QStringLiteral("\\A\\s*/\\* ApplicationStyle\\r?\\n(.*?)\\*/"),
      QRegularExpression::DotMatchesEverythingOption);
  const auto match = header.match(style.sheet);
  if (style.sheet.trimmed().startsWith("/* ApplicationStyle") && !match.hasMatch()) {
    qWarning() << "Application style: malformed metadata header" << file.fileName();
    return false;
  }
  if (match.hasMatch()) {
    QSet<QString> keys;
    const QStringList lines = match.captured(1).split('\n');
    for (const QString &line : lines) {
      if (line.trimmed().isEmpty()) continue;
      const int equals = line.indexOf('=');
      const QString key = line.left(equals).trimmed();
      const QString value = line.mid(equals + 1).trimmed();
      bool valid = equals > 0 && !keys.contains(key) && !value.isEmpty();
      keys.insert(key);
      if (key == "Name") style.name = value;
      else if (key == "Id") style.id = value;
      else if (key == "Colors") {
        valid = valid && (value == "dark" || value == "standard");
        style.darkColors = value == "dark";
      } else if (key == "Default") {
        valid = valid && (value == "true" || value == "false");
        style.isDefault = value == "true";
      } else valid = false;
      if (!valid) {
        qWarning() << "Application style: invalid metadata" << file.fileName() << line;
        return false;
      }
    }
  }
  return true;
}
}

QList<ApplicationStyle> ApplicationStyles::discover(const QString &directory)
{
  QList<ApplicationStyle> result;
  const QDir dir(directory);
  if (!dir.exists()) {
    qWarning() << "Application style directory is missing:" << directory;
    return result;
  }
  QSet<QString> ids;
  bool foundDefault = false;
  const auto files = dir.entryInfoList(QStringList("*.qss"), QDir::Files, QDir::Name);
  for (const QFileInfo &info : files) {
    ApplicationStyle style;
    if (!readStyle(info, style)) continue;
    if (style.id.isEmpty() || ids.contains(style.id)) {
      qWarning() << "Application style: duplicate or empty ID" << style.id << style.fileName;
      continue;
    }
    ids.insert(style.id);
    if (style.isDefault && foundDefault) {
      qWarning() << "Application style: multiple defaults; ignoring default flag in" << style.fileName;
      style.isDefault = false;
    }
    foundDefault = foundDefault || style.isDefault;
    result.append(style);
  }
  return result;
}

ApplicationStyle ApplicationStyles::systemDefault()
{
  ApplicationStyle style;
  // Empty ID is reserved for this always-available fallback.
  style.name = QStringLiteral("System default");
  style.fileName = QStringLiteral(":/style/systemStyle");
  QFile file(style.fileName);
  if (file.open(QIODevice::ReadOnly)) style.sheet = QString::fromUtf8(file.readAll());
  else qWarning() << "Application style: cannot read built-in fallback" << file.errorString();
  return style;
}
