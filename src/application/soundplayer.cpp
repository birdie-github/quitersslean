#include "soundplayer.h"

#include <QDebug>
#include <QFile>
#include <QMediaPlaylist>
#include <QProcess>
#include <QSound>
#include <QStringList>
#include <QTextCodec>
#include <QUrl>

SoundPlayer::SoundPlayer(QObject *parent)
  : QObject(parent)
  , mediaPlayer_(NULL)
  , playlist_(NULL)
{
}

void SoundPlayer::play(const QString &soundPath, bool useMediaPlayer)
{
  if (!QFile::exists(soundPath)) {
    qWarning() << QString("Error playing sound: %1").arg(soundPath);
    return;
  }

  if (useMediaPlayer) {
    if (mediaPlayer_ == NULL) {
      playlist_ = new QMediaPlaylist(this);
      mediaPlayer_ = new QMediaPlayer(this);
      mediaPlayer_->setPlaylist(playlist_);
      connect(mediaPlayer_, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
              this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));
      connect(mediaPlayer_, SIGNAL(error(QMediaPlayer::Error)),
              this, SLOT(mediaError(QMediaPlayer::Error)));
    }

    playlist_->addMedia(QUrl::fromLocalFile(soundPath));
    if (playlist_->currentIndex() == -1) {
      playlist_->setCurrentIndex(1);
      mediaPlayer_->play();
    }
    return;
  }

#if defined(Q_OS_WIN)
  QSound::play(soundPath);
#else
  qInfo() << "Launching" << QStringLiteral("play") << "arguments:"
          << (QStringList() << soundPath);
  QProcess::startDetached(QStringLiteral("play"), QStringList() << soundPath);
#endif
}

void SoundPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
  if (status == QMediaPlayer::EndOfMedia)
    playlist_->removeMedia(0);
}

void SoundPlayer::mediaError(QMediaPlayer::Error error)
{
  QTextCodec *codec = QTextCodec::codecForLocale();
  qCritical() << QString("Error Media: %1 - %2").
                 arg(error).
                 arg(codec->toUnicode(mediaPlayer_->errorString().toUtf8()));
}
