#include "audiomodel.h"

AudioModel::AudioModel(QObject *parent)
    : QObject(parent)
    , playback_timer_(new QTimer(this))
{
    // 设置计时器
    connect(playback_timer_, &QTimer::timeout, this, &AudioModel::SlotPlaybackUpdate);
}

AudioModel::~AudioModel()
{
    StopPlayback();
}

bool AudioModel::LoadWavFile(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    // WAV文件头解析
    struct WavHeader {
        char riff[4];
        uint32_t file_size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        char data[4];
        uint32_t data_size;
    };
    WavHeader header;
    // 读取WAV头部信息，检查返回读取的字节数是否正确
    if (file.read(reinterpret_cast<char *>(&header), sizeof(WavHeader)) != sizeof(WavHeader)) {
        return false;
    }
    // 验证WAV格式
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        return false;
    }
    // 设置播放格式
    playback_format_.setChannelCount(header.num_channels);
    playback_format_.setSampleRate(header.sample_rate);
    playback_format_.setSampleFormat(header.bits_per_sample == 16 ? QAudioFormat::Int16 : QAudioFormat::Float);
    // 读取音频数据
    playback_data_ = file.read(header.data_size);
    // 计算总时长（秒）
    const int bytes_per_second = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    playback_total_duration_ = playback_data_.size() / bytes_per_second;

    return !playback_data_.isEmpty();
}

bool AudioModel::StartPlayback()
{
    if (playback_data_.isEmpty()) {
        return false;
    }
    // 获取默认音频输出设备
    const auto output_device = QMediaDevices::defaultAudioOutput();
    if (!output_device.isFormatSupported(playback_format_)) {
        return false;
    }
    // 创建音频输出
    audio_sink_ = new QAudioSink(output_device, playback_format_, this);
    // 创建播放缓冲区
    playback_buffer_ = new QBuffer(&playback_data_, this);
    playback_buffer_->open(QIODevice::ReadOnly);
    // 开始播放
    audio_sink_->start(playback_buffer_);
    // 启动进度定时器
    playback_current_position_ = 0;
    playback_timer_->start(1000); // 每秒更新一次进度

    return true;
}

void AudioModel::StopPlayback()
{
    playback_timer_->stop();
    if (audio_sink_) {
        audio_sink_->stop();
        delete audio_sink_;
        audio_sink_ = nullptr;
    }
    if (playback_buffer_) {
        playback_buffer_->close();
        delete playback_buffer_;
        playback_buffer_ = nullptr;
    }
    playback_current_position_ = 0;
    emit PlaybackPositionChanged(0, playback_total_duration_);
}

void AudioModel::PausePlayback()
{
    if (audio_sink_->state() == QAudio::ActiveState) {
        audio_sink_->suspend();
        playback_timer_->stop();
    } else if (audio_sink_->state() == QAudio::SuspendedState) {
        audio_sink_->resume();
        playback_timer_->start(1000);
    }
}
