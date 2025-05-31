#include "networkmodel.h"
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QDebug>

NetworkModel::NetworkModel(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , receive_directory_(QDir::currentPath())
    , receive_state_(kNotReceiving)
    , expected_file_size_(0)
    , bytes_received_(0)
    , receive_file_(nullptr)
{
    connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkModel::onErrorOccurred);
    connect(socket_, &QTcpSocket::readyRead, this, &NetworkModel::onReadyRead);
}

NetworkModel::~NetworkModel()
{
    if (receive_file_) {
        receive_file_->close();
        delete receive_file_;
        receive_file_ = nullptr;
    }
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
    ResetReceiveState();
    emit connectionChanged(Disconnected);
}

void NetworkModel::SetReceiveDirectory(const QString &directory_path)
{
    if (QDir(directory_path).exists()) {
        receive_directory_ = directory_path;
    }
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
    ResetReceiveState();
    emit connectionChanged(Error);
}

void NetworkModel::onReadyRead()
{
    if (socket_->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "收到数据，当前缓冲区大小:" << receive_buffer_.size() << "新数据大小:" << socket_->bytesAvailable();
        ProcessIncomingData();
    }
}

void NetworkModel::ProcessIncomingData()
{
    QByteArray data = socket_->readAll();
    receive_buffer_.append(data);
    qDebug() << "ProcessIncomingData: 新接收" << data.size() << "字节，缓冲区总大小:" << receive_buffer_.size() << "状态:" << receive_state_;
    
    if (receive_state_ == kNotReceiving) {
        // 尝试解析文件头信息
        QDataStream stream(receive_buffer_);
        stream.setVersion(QDataStream::Qt_6_0);  // 与发送端保持一致
        
        QString file_name;
        qint64 file_size;
        
        // 尝试读取文件名和文件大小
        stream >> file_name >> file_size;
        
        qDebug() << "尝试解析头部: 文件名=" << file_name << "文件大小=" << file_size << "流状态=" << stream.status();
        
        if (stream.status() == QDataStream::Ok && !file_name.isEmpty() && file_size > 0) {
            // 成功解析头部信息
            expected_file_name_ = file_name;
            expected_file_size_ = file_size;
            
            // 计算头部大小并移除
            qint64 header_size = stream.device()->pos();
            receive_buffer_.remove(0, header_size);
            
            qDebug() << "头部解析成功: 文件名=" << expected_file_name_ << "大小=" << expected_file_size_ << "头部大小=" << header_size;
            
            // 准备接收文件
            QString save_path = QDir(receive_directory_).filePath(QFileInfo(expected_file_name_).fileName());
            receive_file_ = new QFile(save_path, this);
            
            if (!receive_file_->open(QIODevice::WriteOnly)) {
                emit fileReceiveError("无法创建文件: " + save_path);
                ResetReceiveState();
                return;
            }
            
            receive_state_ = kReceiving;
            bytes_received_ = 0;
            
            emit fileReceiveStarted(expected_file_name_, expected_file_size_);
        }
    }    
    if (receive_state_ == kReceiving && !receive_buffer_.isEmpty()) {
        // 写入文件数据
        qint64 bytes_to_write = qMin(static_cast<qint64>(receive_buffer_.size()), 
                                    expected_file_size_ - bytes_received_);
        
        qDebug() << "写入文件数据: 缓冲区大小=" << receive_buffer_.size() << "计划写入=" << bytes_to_write << "已接收=" << bytes_received_ << "总大小=" << expected_file_size_;
        
        if (bytes_to_write > 0) {
            qint64 bytes_written = receive_file_->write(receive_buffer_.left(bytes_to_write));
            if (bytes_written == -1) {
                emit fileReceiveError("写入文件失败: " + receive_file_->errorString());
                ResetReceiveState();
                return;
            }
            
            bytes_received_ += bytes_written;
            receive_buffer_.remove(0, bytes_written);
            
            qDebug() << "文件写入成功: 写入=" << bytes_written << "总已接收=" << bytes_received_ << "进度=" << (bytes_received_ * 100 / expected_file_size_) << "%";
            
            emit fileReceiveProgress(bytes_received_, expected_file_size_);
            
            // 检查是否接收完成
            if (bytes_received_ >= expected_file_size_) {
                QString saved_path = receive_file_->fileName();
                receive_file_->close();
                delete receive_file_;
                receive_file_ = nullptr;
                
                qDebug() << "文件接收完成: " << saved_path;
                emit fileReceiveCompleted(saved_path);
                receive_state_ = kNotReceiving;
            }
        }
    }
}

void NetworkModel::ResetReceiveState()
{
    if (receive_file_) {
        receive_file_->close();
        delete receive_file_;
        receive_file_ = nullptr;
    }
    
    receive_state_ = kNotReceiving;
    expected_file_name_.clear();
    expected_file_size_ = 0;
    bytes_received_ = 0;
    receive_buffer_.clear();
}
