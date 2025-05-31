#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , network_model(new NetworkModel(this))
{
    ui->setupUi(this);
    // 连接网络模型信号
    connect(network_model, &NetworkModel::connectionChanged, this, &MainWindow::onConnectionChanged);
    connect(network_model, &NetworkModel::fileReceiveStarted, this, &MainWindow::onFileReceiveStarted);
    connect(network_model, &NetworkModel::fileReceiveProgress, this, &MainWindow::onFileReceiveProgress);
    connect(network_model, &NetworkModel::fileReceiveCompleted, this, &MainWindow::onFileReceiveCompleted);
    connect(network_model, &NetworkModel::fileReceiveError, this, &MainWindow::onFileReceiveError);
    // 设置默认保存目录
    network_model->SetReceiveDirectory(QDir::currentPath());
}

MainWindow::~MainWindow()
{
    // 确保在程序退出前断开连接
    if (network_model->IsConnected()) {
        network_model->CloseConnection();
    }
    delete ui;
}

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
        network_model->StartConnection(ip, port);
    } else {
        // 断开连接
        network_model->CloseConnection();
    }
}

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
        QString error = network_model->GetErrorMessage();
        ui->textBrowser_client_info->append("连接失败: " + error);
        QMessageBox::warning(this, "连接失败", error);
        ui->btn_connect->setChecked(false);
        ui->btn_connect->setText("建立连接");
        break;
    }
}

void MainWindow::on_btn_select_save_dir_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, "选择文件保存目录", QDir::currentPath());
    if (!directory.isEmpty()) {
        network_model->SetReceiveDirectory(directory);
        ui->textBrowser_client_info->append("文件保存目录已设置为: " + directory);
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
