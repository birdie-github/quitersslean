#ifndef NEWSRETENTION_H
#define NEWSRETENTION_H

#include <QTimeZone>
#include <QDateTime>
#include <QString>

namespace NewsRetention {
// parseDate() normalizes publisher dates to this UTC format. Older versions
// stored a synthetic receipt timestamp with a trailing Z when no date existed.
// Do not mistake those synthetic dates for publisher dates during cleanup.
inline QDateTime publicationDate(const QString &value)
{
  if (value.size() != 19) return QDateTime();
  QDateTime date = QDateTime::fromString(value, QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  date.setTimeZone(QTimeZone(QTimeZone::UTC));
#else
  date.setTimeSpec(Qt::UTC);
#endif
  return date;
}

inline QDateTime cutoff(int days)
{
  return QDateTime::currentDateTimeUtc().addDays(-qMax(0, days));
}

inline bool expired(const QString &published, const QDateTime &limit)
{
  const QDateTime date = publicationDate(published);
  return limit.isValid() && date.isValid() && date < limit;
}
}

#endif // NEWSRETENTION_H
