#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , network_model_(new NetworkModel(this))
    , txt_model_(new TxtModel(this))
    , audio_model_(new AudioModel(this))
{
    ui->setupUi(this);
    ui->label_sample_rate->setText(" 采样率: " + QString::number(txt_model_->kSampleRate) + " Hz"
                                   + "                    "
                                   + " 传信率: " + QString::number(txt_model_->kSampleRate / txt_model_->kSamplesPerBit) + " bps"
                                   + "                    "
                                   + " 载波: " + QString::number(txt_model_->kCarrierFreq) + " Hz");
    // 连接网络模型信号
    connect(network_model_, &NetworkModel::connectionChanged, this, &MainWindow::onConnectionChanged);
    connect(network_model_, &NetworkModel::fileReceiveStarted, this, &MainWindow::onFileReceiveStarted);
    connect(network_model_, &NetworkModel::fileReceiveProgress, this, &MainWindow::onFileReceiveProgress);
    connect(network_model_, &NetworkModel::fileReceiveCompleted, this, &MainWindow::onFileReceiveCompleted);
    connect(network_model_, &NetworkModel::fileReceiveError, this, &MainWindow::onFileReceiveError);
    // 设置默认保存目录
    QString receive_dir = QDir::currentPath() + "/Received Files";
    QDir dir(receive_dir);
    if (!dir.exists()) {
        dir.mkpath(receive_dir);
    }
    network_model_->set_receive_directory(receive_dir);
    
    // 连接音频播放相关信号
    connect(audio_model_, &AudioModel::PlaybackPositionChanged, this, &MainWindow::UpdatePlaybackProgress);
    connect(audio_model_, &AudioModel::PlaybackFinished, this, &MainWindow::OnPlaybackFinished);
}

MainWindow::~MainWindow()
{
    // 确保在程序退出前断开连接
    if (network_model_->IsConnected()) {
        network_model_->CloseConnection();
    }
    delete ui;
}

// Button Slots
void MainWindow::on_btn_connect_clicked(bool checked)
{
    if (checked) {
        // 尝试连接
        const auto ip = ui->lineEdit_ip->text();
        const auto port = ui->lineEdit_port->text();
        if (ip.isEmpty() || port.isEmpty()) {
            QMessageBox::warning(this, "错误", "IP地址或端口号不能为空！");
            ui->btn_connect->setChecked(false);
            return;
        }
        // 开始连接
        network_model_->StartConnection(ip, port);
    } else {
        // 断开连接
        network_model_->CloseConnection();
    }
}

void MainWindow::on_btn_load_received_file_clicked()
{
    const auto file_name = QFileDialog::getOpenFileName(this, "选择接收的文件", network_model_->get_receive_directory(), "文本文件 (*.txt)");
    if (file_name.isEmpty()) {
        return;
    }
    if (txt_model_->LoadTxtFile(file_name)) {
        ui->textBrowser_original->setText(txt_model_->get_txt_received_data());
        ui->btn_demodulate->setEnabled(true);
    }
}

void MainWindow::on_btn_demodulate_clicked()
{
    txt_model_->DemodulateTxtFile(ui->comboBox_demodulation->currentText());
    const auto &data = txt_model_->get_txt_demodulated_data();
    QString str;
    for (const auto bit : data) {
        str.append(QString::number(bit));
    }
    ui->textBrowser_demodulated->setText(str.trimmed());
    ui->btn_decode->setEnabled(true);
}

void MainWindow::on_btn_decode_clicked()
{
    txt_model_->DecodeTxtFile(ui->comboBox_decoding->currentText());
    ui->textBrowser_decoded->setText(txt_model_->get_txt_recovered_data());
    ui->btn_save_recovered_file->setEnabled(true);
}

void MainWindow::on_btn_save_recovered_file_clicked()
{
    const auto file_name = QFileDialog::getSaveFileName(this, "Save Recovered File", "", "Text Files (*.txt)");
    if (!file_name.isEmpty()) {
        txt_model_->SaveRecoverdFile(file_name);
    }
}

// Network Slots
void MainWindow::onConnectionChanged(NetworkModel::ConnectionState state)
{
    // 恢复按钮可用状态
    ui->btn_connect->setEnabled(true);
    switch (state) {
    case NetworkModel::Connected:
        // 只有在真正连接成功时才显示成功信息
        ui->textBrowser_client_info->append("连接成功！");
        ui->btn_connect->setText("断开连接");
        ui->btn_connect->setChecked(true);
        break;
    case NetworkModel::Disconnected:
        ui->textBrowser_client_info->append("连接已断开");
        ui->btn_connect->setText("建立连接");
        ui->btn_connect->setChecked(false);
        break;
    case NetworkModel::Connecting:
        ui->textBrowser_client_info->append("正在连接中...");
        ui->btn_connect->setText("连接中...");
        ui->btn_connect->setEnabled(false);
        break;
    case NetworkModel::Error:
        QString error = network_model_->get_error_message();
        ui->textBrowser_client_info->append("连接失败: " + error);
        QMessageBox::warning(this, "连接失败", error);
        ui->btn_connect->setChecked(false);
        ui->btn_connect->setText("建立连接");
        break;
    }
}

void MainWindow::onFileReceiveStarted(const QString &file_name, qint64 file_size)
{
    ui->textBrowser_client_info->append(QString("开始接收文件: %1 (大小: %2 字节)")
                                       .arg(file_name)
                                       .arg(file_size));
}

void MainWindow::onFileReceiveProgress(qint64 bytes_received, qint64 total_bytes)
{
    int progress = static_cast<int>((bytes_received * 100) / total_bytes);
    ui->textBrowser_client_info->append(QString("接收进度: %1% (%2/%3 字节)")
                                       .arg(progress)
                                       .arg(bytes_received)
                                       .arg(total_bytes));
}

void MainWindow::onFileReceiveCompleted(const QString &saved_file_path)
{
    ui->textBrowser_client_info->append("文件接收完成，已保存到: " + saved_file_path);
    QMessageBox::information(this, "文件接收完成", "文件已成功保存到:\n" + saved_file_path);
}

void MainWindow::onFileReceiveError(const QString &error_message)
{
    ui->textBrowser_client_info->append("文件接收错误: " + error_message);
    QMessageBox::warning(this, "文件接收错误", error_message);
}

// 音频播放功能相关
void MainWindow::on_btn_open_recorded_file_clicked()
{
    const auto file_name = QFileDialog::getOpenFileName(this, "打开WAV文件", network_model_->get_receive_directory(), "WAV文件 (*.wav)");
    if (file_name.isEmpty()) {
        return;
    }
    ui->lineEdit_wav_file_path->setText(file_name);
    if (audio_model_->LoadWavFile(file_name)) {
        const int total_duration = audio_model_->get_playback_total_duration();
        const int minutes = total_duration / 60;
        const int seconds = total_duration % 60;

        ui->label_playback->setText(QString("00:00 / %1:%2")
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0')));
        ui->progressBar_playback->setValue(0);

        QMessageBox::information(this, "文件加载成功",
                                QString("WAV文件已加载: %1\n时长: %2:%3")
                                .arg(QFileInfo(file_name).fileName())
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0')));
        
        ui->btn_play_wav->setEnabled(true);
        ui->btn_pause_wav->setEnabled(false);
        ui->btn_close_wav->setEnabled(false);
    } else {
        ui->lineEdit_wav_file_path->clear();
        QMessageBox::warning(this, "文件加载失败", "无法加载WAV文件，请检查文件格式。");
    }
}

void MainWindow::on_btn_play_wav_clicked()
{
    if (ui->lineEdit_wav_file_path->text().isEmpty()) {
        QMessageBox::warning(this, "播放失败", "请先选择要播放的WAV文件。");
        return;
    }
    if (audio_model_->StartPlayback()) {
        ui->btn_play_wav->setEnabled(false);
        ui->btn_pause_wav->setEnabled(true);
        ui->btn_close_wav->setEnabled(true);
        ui->btn_open_recorded_file->setEnabled(false);
    } else {
        QMessageBox::warning(this, "播放失败", "无法开始播放，请检查音频文件和设备状态。");
    }
}

void MainWindow::on_btn_pause_wav_clicked()
{
    audio_model_->PausePlayback();
    if (audio_model_->IsPlaying()) {
        ui->btn_pause_wav->setText("暂停");
    } else {
        ui->btn_pause_wav->setText("继续");
    }
}

void MainWindow::on_btn_close_wav_clicked()
{
    audio_model_->StopPlayback();
    OnPlaybackFinished();
}

void MainWindow::UpdatePlaybackProgress(int current_seconds, int total_seconds)
{
    const int current_minutes = current_seconds / 60;
    const int current_secs = current_seconds % 60;
    const int total_minutes = total_seconds / 60;
    const int total_secs = total_seconds % 60;
    // 更新时间标签
    ui->label_playback->setText(QString("%1:%2 / %3:%4")
                                .arg(current_minutes, 2, 10, QChar('0'))
                                .arg(current_secs, 2, 10, QChar('0'))
                                .arg(total_minutes, 2, 10, QChar('0'))
                                .arg(total_secs, 2, 10, QChar('0')));
    // 更新进度条
    if (total_seconds > 0) {
        const int progress = (current_seconds * 100) / total_seconds;
        ui->progressBar_playback->setValue(progress);
    }
}

void MainWindow::OnPlaybackFinished()
{
    ui->btn_play_wav->setEnabled(true);
    ui->btn_pause_wav->setEnabled(false);
    ui->btn_close_wav->setEnabled(false);
    ui->btn_pause_wav->setText("暂停");
    ui->btn_open_recorded_file->setEnabled(true);
    ui->progressBar_playback->setValue(0);

    const int total_duration = audio_model_->get_playback_total_duration();
    const int minutes = total_duration / 60;
    const int seconds = total_duration % 60;
    ui->label_playback->setText(QString("00:00 / %1:%2")
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0')));
}
