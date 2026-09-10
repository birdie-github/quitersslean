#ifndef STATUSBARCONTROLLER_H
#define STATUSBARCONTROLLER_H

#include <QObject>
#include <QString>

class QAction;
class QLabel;
class QProgressBar;
class QStatusBar;

class StatusBarController : public QObject
{
public:
  StatusBarController(QStatusBar *statusBar, QAction *stopUpdateAction,
                      QAction *loadImagesAction, QAction *fullScreenAction,
                      QObject *parent = 0);

  void startProgress(int maximum);
  void updateProgress(int remaining);
  void hideProgress();
  void resetProgress();

  void showMessage(const QString &message, int timeout = 0);
  void setCountTexts(const QString &unreadText, const QString &allText);
  QString unreadText() const;
  QString allText() const;
  void setCountsVisible(bool unreadVisible, bool allVisible);

private:
  QStatusBar *statusBar_;
  QProgressBar *progressBar_;
  QLabel *statusUnread_;
  QLabel *statusAll_;
};

#endif // STATUSBARCONTROLLER_H
