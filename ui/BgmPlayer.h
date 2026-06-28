#ifndef BGMPLAYER_H
#define BGMPLAYER_H

#include <QObject>

class QMediaPlayer;
class QAudioOutput;

/// 背景音乐播放器 — 封装 QMediaPlayer 实现循环播放
class BgmPlayer : public QObject
{
    Q_OBJECT

public:
    explicit BgmPlayer(QObject *parent = nullptr);
    ~BgmPlayer() override;

    /// 加载并循环播放指定音频文件
    void play(const QString &filePath);

    /// 暂停
    void pause();

    /// 恢复
    void resume();

    /// 停止
    void stop();

    /// 设置音量 0.0 ~ 1.0
    void setVolume(float vol);

    /// 切换静音
    void toggleMute();

    bool isMuted() const { return m_muted; }

private:
    QMediaPlayer *m_player    = nullptr;
    QAudioOutput *m_audioOut  = nullptr;
    float         m_volume    = 1.0f;
    bool          m_muted     = false;
};

#endif // BGMPLAYER_H
