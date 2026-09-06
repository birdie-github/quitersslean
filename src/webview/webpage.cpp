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
#include "webpage.h"
#include "articlecontent.h"
#include "articleimages.h"
#include <QWebFrame>
#include <QWebSettings>
#include <QNetworkRequest>
#include <QWebHistory>

WebPage::WebPage(QObject *parent) : QWebPage(parent), images_(new ArticleImages(this)) {
  setNetworkAccessManager(images_);
  setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
  setForwardUnsupportedContent(false);
  settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
  settings()->setAttribute(QWebSettings::JavaEnabled, false);
  settings()->setAttribute(QWebSettings::PluginsEnabled, false);
  settings()->setAttribute(QWebSettings::LocalStorageEnabled, false);
  settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, false);
  settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, false);
  settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, false);
  settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
  settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, false);
  settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, false);
  settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
  history()->setMaximumItemCount(0);
}
void WebPage::disconnectObjects() { images_->reset(); disconnect(this); }
void WebPage::resetArticleImages() { images_->reset(); }
QString WebPage::prepareArticle(const QString &html, const QUrl &base, const QString &prefix, bool images) {
  QSet<QUrl> urls;
  const QString result = ArticleContent::sanitize(html, base, prefix, images, &urls);
  images_->allow(urls);
  return result;
}
bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type) {
  if (frame && frame != mainFrame()) return false;
  if (type == NavigationTypeLinkClicked) {
    emit linkClicked(request.url());
    return false;
  }
  // Permit only our substitute document, never arbitrary navigation, forms or reloads.
  return type == NavigationTypeOther && (request.url().isEmpty() || request.url() == QUrl("about:blank") ||
    request.url() == QUrl("https://quiterss.invalid/"));
}
QWebPage *WebPage::createWindow(WebWindowType) { return nullptr; }
