#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QObject>

#include <QMediaPlayer>

class QMediaPlaylist;
class QString;

class SoundPlayer : public QObject
{
  Q_OBJECT
public:
  explicit SoundPlayer(QObject *parent = 0);

  void play(const QString &soundPath, bool useMediaPlayer);

private slots:
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
  void mediaError(QMediaPlayer::Error error);

private:
  QMediaPlayer *mediaPlayer_;
  QMediaPlaylist *playlist_;
};

#endif // SOUNDPLAYER_H
