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
#include "webview.h"
#include "webpage.h"

#include <QApplication>
#include <QInputEvent>
#include <QDebug>
#include <QDrag>
#include <QMimeData>

WebView::WebView(QWidget *parent)
  : QWebView(parent)
  , buttonClick_(0)
  , isLoading_(false)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  setAcceptDrops(false);
  setPage(new WebPage(this));
  QPalette pal(qApp->palette());
  pal.setColor(QPalette::Base, Qt::white);
  setPalette(pal);

  connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished()));
}

void WebView::disconnectObjects()
{
  disconnect(this);
}

void WebView::mousePressEvent(QMouseEvent *event)
{
  buttonClick_ = 0;

  if (event->buttons() == Qt::LeftButton) {
    dragStartPos_ = event->pos();
  }

  QWebView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    emit showContextMenu(event->pos());
    event->accept();
    return;
  }
  if (event->button() == Qt::MiddleButton ||
      (event->button() == Qt::LeftButton && event->modifiers() != Qt::NoModifier)) {
    const QUrl link = page()->mainFrame()->hitTestContent(event->pos()).linkUrl();
    if (!link.isEmpty()) {
      emit linkClicked(link);
      event->accept();
      return;
    }
  }
  QWebView::mouseReleaseEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event)
{
  if (event->modifiers() == Qt::ControlModifier) {
    if (event->delta() > 0) {
      if (zoomFactor() < 5.0)
        setZoomFactor(zoomFactor()+0.1);
    }
    else {
      if (zoomFactor() > 0.3)
        setZoomFactor(zoomFactor()-0.1);
    }
    event->accept();
    return;
  }
  QWebView::wheelEvent(event);
}

void WebView::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() != Qt::LeftButton) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  QSize viewSize;
  viewSize.setWidth(page()->viewportSize().width() -
                    page()->mainFrame()->scrollBarGeometry(Qt::Vertical).width());
  viewSize.setHeight(page()->viewportSize().height() -
                     page()->mainFrame()->scrollBarGeometry(Qt::Horizontal).height());
  if ((dragStartPos_.x() > viewSize.width()) || (dragStartPos_.y() > viewSize.height())) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  int manhattanLength = (event->pos() - dragStartPos_).manhattanLength();
  if (manhattanLength <= QApplication::startDragDistance()) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  const QWebHitTestResult &hitTest = page()->mainFrame()->hitTestContent(dragStartPos_);
  if (hitTest.linkUrl().isEmpty()) {
    QWebView::mouseMoveEvent(event);
    return;
  }

  QDrag *drag = new QDrag(this);
  QMimeData *mime = new QMimeData;
  mime->setUrls(QList<QUrl>() << hitTest.linkUrl());
  mime->setText(hitTest.linkUrl().toString());

  drag->setMimeData(mime);
  drag->exec();
}

void WebView::slotLoadStarted()
{
  isLoading_ = true;

}

void WebView::slotLoadFinished()
{
  isLoading_ = false;
}
