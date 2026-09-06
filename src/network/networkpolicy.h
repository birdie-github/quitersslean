#ifndef NETWORKPOLICY_H
#define NETWORKPOLICY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace NetworkPolicy {
inline bool isHttpUrl(const QUrl &url)
{
  return url.isValid() && !url.host().isEmpty() &&
      (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
}

inline bool isSafeRedirect(const QUrl &source, const QUrl &target)
{
  return isHttpUrl(target) &&
      !(source.scheme() == QLatin1String("https") && target.scheme() == QLatin1String("http"));
}

// Keep explicit local-file feeds usable, but never redirect network requests to them.
inline bool isRequestUrl(const QUrl &url)
{
  return isHttpUrl(url) || (url.isValid() && url.isLocalFile() && url.host().isEmpty());
}

class RejectedReply : public QNetworkReply
{
public:
  RejectedReply(QNetworkAccessManager::Operation operation, const QNetworkRequest &request,
                QObject *parent) : QNetworkReply(parent)
  {
    setOperation(operation);
    setRequest(request);
    setUrl(request.url());
    open(QIODevice::ReadOnly);
    setError(QNetworkReply::ProtocolUnknownError,
             tr("Unsupported URL scheme: %1").arg(request.url().scheme()));
    QTimer::singleShot(0, this, [this]() { complete(); });
  }

  void abort() override
  {
    if (isFinished()) return;
    setError(QNetworkReply::OperationCanceledError, tr("Operation canceled"));
    complete();
  }

protected:
  qint64 readData(char *, qint64) override { return -1; }

private:
  void complete()
  {
    if (isFinished()) return;
    setFinished(true);
    emit errorOccurred(error());
    emit finished();
  }
};
}

#endif // NETWORKPOLICY_H
