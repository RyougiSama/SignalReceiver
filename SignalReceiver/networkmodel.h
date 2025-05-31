#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QDataStream>

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

    enum ReceiveState {
        kNotReceiving,
        kReceiving
    };

public:
    NetworkModel(QObject *parent);
    ~NetworkModel();

    void StartConnection(const QString &ip, const QString &port);
    void CloseConnection();

    void set_receive_directory(const QString &directory_path);

    bool IsConnected() const { return socket_->state() == QAbstractSocket::ConnectedState; }
    QString get_error_message() const { return error_message_.isEmpty() ? socket_->errorString() : error_message_; }
    ReceiveState get_receive_state() const { return receive_state_; }
    QString get_receive_directory() const { return receive_directory_; }

signals:
    void connectionChanged(ConnectionState state);
    void fileReceiveStarted(const QString &file_name, qint64 file_size);
    void fileReceiveProgress(qint64 bytes_received, qint64 total_bytes);
    void fileReceiveCompleted(const QString &saved_file_path);
    void fileReceiveError(const QString &error_message);

private slots:
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReadyRead();

private:
    bool IsValidIPv4(const QString &ip) const { QHostAddress address(ip); return !address.isNull() && address.protocol() == QAbstractSocket::IPv4Protocol; }
    bool IsValidPort(const QString &port, quint16 &port_num) const { bool ok{ false }; auto value = port.toUShort(&ok); if (ok && value > 0 && value <= 65535) { port_num = static_cast<quint16>(value); return true; } return false; }
    void ProcessIncomingData();
    void ResetReceiveState();

private:
    QTcpSocket *socket_;
    QString error_message_;
    QString receive_directory_;
    ReceiveState receive_state_;

    // 文件接收相关
    QString expected_file_name_;
    qint64 expected_file_size_;
    qint64 bytes_received_;
    QFile *receive_file_;
    QByteArray receive_buffer_;
};
