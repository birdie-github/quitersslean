/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
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
#include "articleimages.h"
#include "articlecontent.h"
#include "mainapplication.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookieJar>
#include <QBuffer>
#include <QImageReader>
#include <QTimer>
#include <QPointer>
#include <cstring>

namespace {
class NoCookies : public QNetworkCookieJar {
public:
  explicit NoCookies(QObject *parent) : QNetworkCookieJar(parent) {}
  bool setCookiesFromUrl(const QList<QNetworkCookie> &, const QUrl &) override { return false; }
};
class ImageReply : public QNetworkReply {
public:
  ImageReply(const QNetworkRequest &request, QNetworkAccessManager *fetcher, bool allowed, QObject *parent)
    : QNetworkReply(parent), fetcher_(fetcher), pending_(nullptr), offset_(0), redirects_(0), done_(false) {
    setRequest(request);
    setUrl(request.url());
    setOperation(QNetworkAccessManager::GetOperation);
    open(QIODevice::ReadOnly);
    QTimer::singleShot(0, this, [this, allowed]() {
      if (done_) return;
      if (allowed) fetch(url());
      else finish(QNetworkReply::ContentAccessDenied, "Article resource blocked");
    });
    QTimer::singleShot(30000, this, [this]() { if (!done_) finish(QNetworkReply::TimeoutError, "Article image timed out"); });
  }
  ~ImageReply() override {
    if (pending_) { pending_->disconnect(this); pending_->abort(); pending_->deleteLater(); }
  }
  void abort() override { if (!done_) finish(QNetworkReply::OperationCanceledError, "Article image canceled"); }
  qint64 bytesAvailable() const override { return (done_ ? body_.size() - offset_ : 0) + QNetworkReply::bytesAvailable(); }
  bool isSequential() const override { return true; }
protected:
  qint64 readData(char *data, qint64 maxSize) override {
    if (!done_) return 0;
    const qint64 size = qMin(maxSize, qint64(body_.size()) - offset_);
    if (size <= 0) return done_ ? -1 : 0;
    std::memcpy(data, body_.constData() + offset_, size);
    offset_ += size;
    return size;
  }
private:
  void finish(NetworkError error = NoError, const QString &message = QString()) {
    if (done_) return;
    done_ = true;
    if (pending_) {
      pending_->disconnect(this);
      if (pending_->isRunning()) pending_->abort();
      pending_->deleteLater();
      pending_ = nullptr;
    }
    if (error != NoError) { body_.clear(); setError(error, message); }
    setFinished(true);
    if (error != NoError) emit errorOccurred(error);
    else { emit metaDataChanged(); emit readyRead(); }
    emit finished();
  }
  void fetch(const QUrl &target) {
    if (done_) return;
    QNetworkRequest request(target);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::AuthenticationReuseAttribute, QNetworkRequest::Manual);
    request.setRawHeader("Accept", "image/png,image/jpeg,image/gif,image/webp,image/bmp");
    // New request: never carry feed credentials, Referer, cookies, or publisher headers.
    pending_ = fetcher_->get(request);
    connect(pending_.data(), &QNetworkReply::readyRead, this, [this]() {
      if (!pending_) return;
      body_ += pending_->readAll();
      if (body_.size() > 16 * 1024 * 1024) finish(QNetworkReply::ContentAccessDenied, "Article image exceeds size limit");
    });
    connect(pending_.data(), &QNetworkReply::finished, this, [this]() {
      if (!pending_ || done_) return;
      if (pending_->error() != NoError) { finish(pending_->error(), pending_->errorString()); return; }
      const QUrl redirect = pending_->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (!redirect.isEmpty()) {
        const QUrl target = pending_->url().resolved(redirect);
        if (++redirects_ > 5 || !ArticleContent::isRemoteImage(target) ||
            (pending_->url().scheme() == "https" && target.scheme() != "https")) {
          finish(QNetworkReply::ContentAccessDenied, "Article image redirect blocked"); return;
        }
        pending_->deleteLater(); pending_ = nullptr; body_.clear();
        fetch(target); return;
      }
      body_ += pending_->readAll();
      if (body_.size() > 16 * 1024 * 1024) { finish(QNetworkReply::ContentAccessDenied, "Article image exceeds size limit"); return; }
      QBuffer buffer(&body_); buffer.open(QIODevice::ReadOnly);
      QImageReader reader(&buffer);
      const QByteArray format = reader.format().toLower();
      const QSize size = reader.size();
      if (!(format == "png" || format == "jpeg" || format == "gif" || format == "webp" || format == "bmp") ||
          !size.isValid() || qint64(size.width()) * size.height() > 64 * 1024 * 1024) {
        finish(QNetworkReply::ContentAccessDenied, "Unsupported article image"); return;
      }
      setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
      setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QStringLiteral("OK"));
      setHeader(QNetworkRequest::ContentTypeHeader, "image/" + QString::fromLatin1(format));
      setHeader(QNetworkRequest::ContentLengthHeader, body_.size());
      finish();
    });
  }
  QNetworkAccessManager *fetcher_;
  QPointer<QNetworkReply> pending_;
  QByteArray body_;
  qint64 offset_;
  int redirects_;
  bool done_;
};
}
ArticleImages::ArticleImages(QObject *parent) : QNetworkAccessManager(parent), fetcher_(new QNetworkAccessManager(this)) {
  fetcher_->setCookieJar(new NoCookies(fetcher_));
  fetcher_->setProxy(mainApp->networkProxy());
}
void ArticleImages::reset() {
  allowed_.clear();
  const auto replies = findChildren<QNetworkReply *>(QString(), Qt::FindDirectChildrenOnly);
  for (QNetworkReply *reply : replies) reply->abort();
}
QNetworkReply *ArticleImages::createRequest(Operation operation, const QNetworkRequest &request, QIODevice *) {
  const QUrl url = request.url();
  const bool trustedIcon = url.scheme() == "qrc" && (url.path().startsWith("/images/") || url.path().startsWith("/share/"));
  const bool allowed = operation == GetOperation &&
    (trustedIcon || ArticleContent::isInlineImage(url) ||
     (allowed_.contains(url) && ArticleContent::isRemoteImage(url)));
  fetcher_->setProxy(mainApp->networkProxy());
  return new ImageReply(request, fetcher_, allowed, this);
}
