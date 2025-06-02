#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "networkmodel.h"
#include "txtmodel.h"
#include "audiomodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_connect_clicked(bool checked);
    void onConnectionChanged(NetworkModel::ConnectionState state);
    void onFileReceiveStarted(const QString &file_name, qint64 file_size);
    void onFileReceiveProgress(qint64 bytes_received, qint64 total_bytes);
    void onFileReceiveCompleted(const QString &saved_file_path);
    void onFileReceiveError(const QString &error_message);

private:
    Ui::MainWindowClass *ui;
    NetworkModel *network_model_;
    TxtModel *txt_model_;
    AudioModel *audio_model_;

private slots:
    // 文本操作相关
    void on_btn_load_received_file_clicked();
    void on_btn_demodulate_clicked();
    void on_btn_decode_clicked();
    void on_btn_save_recovered_file_clicked();
    
    // 音频操作相关
    void on_btn_open_recorded_file_clicked();
    void on_btn_play_wav_clicked();
    void on_btn_pause_wav_clicked();
    void on_btn_close_wav_clicked();
    void UpdatePlaybackProgress(int current_seconds, int total_seconds);
    void OnPlaybackFinished();
};
