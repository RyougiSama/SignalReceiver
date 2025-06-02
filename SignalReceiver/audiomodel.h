#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QTimer>
#include <QFile>
#include <QAudioSink>
#include <QBuffer>

class AudioModel  : public QObject
{
    Q_OBJECT

public:
    AudioModel(QObject *parent);
    ~AudioModel();

    // 播放
    bool LoadWavFile(const QString &file_path);
    bool StartPlayback();
    void StopPlayback();
    void PausePlayback();
    bool IsPlaying() const { return audio_sink_ && audio_sink_->state() == QAudio::ActiveState; }
    // 获取私有变量值
    int get_playback_total_duration() const { return playback_total_duration_; }

private:
    // 播放相关
    QAudioSink *audio_sink_{ nullptr };
    QBuffer *playback_buffer_{ nullptr };
    QByteArray playback_data_;
    QAudioFormat playback_format_;
    QTimer *playback_timer_;
    int playback_total_duration_{ 0 };
    int playback_current_position_{ 0 };

private slots:
    // 播放进度更新
    void SlotPlaybackUpdate() {
        playback_current_position_++;
        emit PlaybackPositionChanged(playback_current_position_, playback_total_duration_);
        // 检查是否播放完成
        if (playback_current_position_ >= playback_total_duration_) {
            StopPlayback();
            emit PlaybackFinished();
        }
    }

signals:
    // 播放进度更新信号
    void PlaybackPositionChanged(int current_seconds, int total_seconds);
    // 播放完成信号
    void PlaybackFinished();
};

