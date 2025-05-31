#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

class NetworkModel : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

public:
    NetworkModel(QObject *parent);
    ~NetworkModel();

    void StartConnection(const QString &ip, const QString &port);
    void CloseConnection();

    bool IsValidIPv4(const QString &ip) const { QHostAddress address(ip); return !address.isNull() && address.protocol() == QAbstractSocket::IPv4Protocol; }
    bool IsValidPort(const QString &port, quint16 &port_num) const { bool ok{ false }; auto value = port.toUShort(&ok); if (ok && value > 0 && value <= 65535) { port_num = static_cast<quint16>(value); return true; } return false; }
    bool IsConnected() const { return socket_->state() == QAbstractSocket::ConnectedState; }
    QString GetErrorMessage() const { return error_message_.isEmpty() ? socket_->errorString() : error_message_; }

signals:
    void connectionChanged(ConnectionState state);

private slots:
    void onConnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onConnectionTimeout();

private:
    QTcpSocket *socket_;
    QString error_message_;
    QTimer *connection_timer_;
};
