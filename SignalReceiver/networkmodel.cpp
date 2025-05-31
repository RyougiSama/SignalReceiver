#include "networkmodel.h"

NetworkModel::NetworkModel(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkModel::onErrorOccurred);
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
    if (socket_->state() == QAbstractSocket::ConnectedState || socket_->state() == QAbstractSocket::ConnectingState) {
        socket_->abort();
    }
    // 告知UI正在连接中
    emit connectionChanged(Connecting);
    // 开始同步连接
    socket_->connectToHost(ip, port_num);
    if (socket_->waitForConnected(5000)) { // 5秒超时
        emit connectionChanged(Connected);
    } else {
        error_message_ = socket_->errorString();
        if (error_message_.isEmpty() && socket_->state() != QAbstractSocket::ConnectedState) {
            error_message_ = "连接超时或发生未知错误";
        }
        socket_->abort();
        emit connectionChanged(Error);
    }
}

void NetworkModel::CloseConnection()
{
    if (socket_->state() == QAbstractSocket::ConnectedState || socket_->state() == QAbstractSocket::ConnectingState) {
        socket_->disconnectFromHost();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->abort();
        }
    }
    emit connectionChanged(Disconnected);
}

void NetworkModel::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        if (error_message_.isEmpty()) {
            error_message_ = socket_->errorString();
        }
    } else {
        error_message_ = socket_->errorString();
    }
    emit connectionChanged(Error);
}
