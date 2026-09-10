#include "statusbarcontroller.h"

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolButton>

StatusBarController::StatusBarController(QStatusBar *statusBar,
                                         QAction *stopUpdateAction,
                                         QAction *loadImagesAction,
                                         QAction *fullScreenAction,
                                         QObject *parent)
  : QObject(parent)
  , statusBar_(statusBar)
  , progressBar_(new QProgressBar(statusBar))
  , statusUnread_(new QLabel(statusBar))
  , statusAll_(new QLabel(statusBar))
{
#if defined(HAVE_X11) || defined(Q_OS_MAC)
  statusBar_->setStyleSheet(QString("QStatusBar::item {border-right: 1px solid %1;"
                                    "margin: 1px;}").
                            arg(qApp->palette().color(QPalette::Dark).name()));
#endif

  progressBar_->setObjectName("progressBar_");
  progressBar_->setFormat("%p%");
  progressBar_->setAlignment(Qt::AlignCenter);
  progressBar_->setFixedSize(100, 15);
  progressBar_->setRange(0, 0);
  progressBar_->setValue(0);
  progressBar_->hide();

  QToolButton *stopUpdateButton = new QToolButton(progressBar_);
  stopUpdateButton->setFocusPolicy(Qt::NoFocus);
  stopUpdateButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  stopUpdateButton->setFixedSize(15, 15);
  stopUpdateButton->setCursor(Qt::ArrowCursor);
  stopUpdateButton->setDefaultAction(stopUpdateAction);
  stopUpdateButton->setStyleSheet(
        "QToolButton { border: none; padding: 0px; background: none; }"
        "QToolButton:hover { background: rgba(150, 150, 150, 60) }");
  stopUpdateButton->move(progressBar_->rect().right() - stopUpdateButton->sizeHint().width(),
                         progressBar_->rect().top());

  QToolButton *loadImagesButton = new QToolButton(statusBar_);
  loadImagesButton->setFocusPolicy(Qt::NoFocus);
  loadImagesButton->setIconSize(QSize(16, 16));
  loadImagesButton->setDefaultAction(loadImagesAction);
  loadImagesButton->setStyleSheet(
        "QToolButton { border: none; padding: 0px; background: none; }");

  QToolButton *fullScreenButton = new QToolButton(statusBar_);
  fullScreenButton->setFocusPolicy(Qt::NoFocus);
  fullScreenButton->setIconSize(QSize(16, 16));
  fullScreenButton->setDefaultAction(fullScreenAction);
  fullScreenButton->setStyleSheet(
        "QToolButton { border: none; padding: 0px; background: none; }");

  statusBar_->addPermanentWidget(progressBar_);
  statusUnread_->hide();
  statusBar_->addPermanentWidget(statusUnread_);
  statusAll_->hide();
  statusBar_->addPermanentWidget(statusAll_);
  statusBar_->addPermanentWidget(loadImagesButton);
  statusBar_->addPermanentWidget(fullScreenButton);
  statusBar_->show();
}

void StatusBarController::startProgress(int maximum)
{
  progressBar_->setMaximum(maximum);
  progressBar_->show();
}

void StatusBarController::updateProgress(int remaining)
{
  if (progressBar_->isVisible())
    progressBar_->setValue(progressBar_->maximum() - remaining);
}

void StatusBarController::hideProgress()
{
  progressBar_->hide();
}

void StatusBarController::resetProgress()
{
  progressBar_->hide();
  progressBar_->setMaximum(0);
  progressBar_->setValue(0);
}

void StatusBarController::showMessage(const QString &message, int timeout)
{
  statusBar_->showMessage(message, timeout);
}

void StatusBarController::setCountTexts(const QString &unreadText, const QString &allText)
{
  statusUnread_->setText(unreadText);
  statusAll_->setText(allText);
}

QString StatusBarController::unreadText() const
{
  return statusUnread_->text();
}

QString StatusBarController::allText() const
{
  return statusAll_->text();
}

void StatusBarController::setCountsVisible(bool unreadVisible, bool allVisible)
{
  statusUnread_->setVisible(unreadVisible);
  statusAll_->setVisible(allVisible);
}
