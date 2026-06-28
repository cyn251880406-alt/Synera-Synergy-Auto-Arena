#include "ui/BgmPlayer.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QDebug>

BgmPlayer::BgmPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOut = new QAudioOutput(this);

    m_player->setAudioOutput(m_audioOut);
    m_player->setLoops(QMediaPlayer::Infinite);
    m_audioOut->setVolume(m_volume);

    // 错误诊断
    connect(m_player, &QMediaPlayer::errorOccurred, this,
        [](QMediaPlayer::Error error, const QString &errorString) {
            qDebug() << "[BGM] Error:" << error << errorString;
        });

    // 状态变化诊断
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
        [](QMediaPlayer::PlaybackState state) {
            qDebug() << "[BGM] State changed to:" << (int)state;
        });
}

BgmPlayer::~BgmPlayer()
{
    m_player->stop();
}

void BgmPlayer::play(const QString &filePath)
{
    qDebug() << "[BGM] Loading:" << filePath;
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->play();
}

void BgmPlayer::pause()
{
    m_player->pause();
}

void BgmPlayer::resume()
{
    if (m_player->playbackState() == QMediaPlayer::PausedState)
        m_player->play();
}

void BgmPlayer::stop()
{
    m_player->stop();
}

void BgmPlayer::setVolume(float vol)
{
    m_volume = qBound(0.0f, vol, 1.0f);
    if (!m_muted)
        m_audioOut->setVolume(m_volume);
}

void BgmPlayer::toggleMute()
{
    m_muted = !m_muted;
    m_audioOut->setVolume(m_muted ? 0.0f : m_volume);
}
