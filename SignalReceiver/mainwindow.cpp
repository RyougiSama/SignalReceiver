#include "mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , network_model(new NetworkModel(this))
{
    ui->setupUi(this);
    // 连接网络模型信号
    connect(network_model, &NetworkModel::connectionChanged, this, &MainWindow::onConnectionChanged);
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
