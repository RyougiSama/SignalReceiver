#include "networkmodel.h"

NetworkModel::NetworkModel(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , connection_timer_(new QTimer(this))
{
    connect(socket_, &QTcpSocket::connected, this, &NetworkModel::onConnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkModel::onErrorOccurred);
    // 设置连接超时
    connection_timer_->setSingleShot(true);
    connection_timer_->setInterval(5000); // 5秒超时
    connect(connection_timer_, &QTimer::timeout, this, &NetworkModel::onConnectionTimeout);
}

NetworkModel::~NetworkModel()
{
}

void NetworkModel::StartConnection(const QString &ip, const QString &port)
{
    // 清除旧的错误信息
    error_message_.clear();
    // 验证IP和端口
    if (!IsValidIPv4(ip)) {
        error_message_ = "无效的IPv4地址格式";
        emit connectionChanged(Error);
        return;
    }
    quint16 port_num{ 0 };
    if (!IsValidPort(port, port_num)) {
        error_message_ = "无效的端口号(1-65535)";
        emit connectionChanged(Error);
        return;
    }
    // 如果已连接，先断开
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->abort();
    }
    // 告知UI正在连接中
    emit connectionChanged(Connecting);
    // 开始异步连接
    socket_->connectToHost(ip, port_num);
    // 启动超时定时器
    connection_timer_->start();
}

void NetworkModel::CloseConnection()
{
    // 停止超时计时器
    connection_timer_->stop();
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->disconnectFromHost();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->abort();
        }
    }
    emit connectionChanged(Disconnected);
}

void NetworkModel::onConnected()
{
    // 停止超时计时器
    connection_timer_->stop();
    // 只有在真正连接成功时才发送Connected信号
    emit connectionChanged(Connected);
}

void NetworkModel::onErrorOccurred(QAbstractSocket::SocketError error)
{
    // 停止超时计时器
    connection_timer_->stop();
    error_message_ = socket_->errorString();
    emit connectionChanged(Error);
}

void NetworkModel::onConnectionTimeout()
{
    // 如果计时器触发，说明连接超时
    socket_->abort();
    error_message_ = "连接超时，请检查服务器是否可达";
    emit connectionChanged(Error);
}
