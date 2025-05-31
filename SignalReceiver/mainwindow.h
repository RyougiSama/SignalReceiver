#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "networkmodel.h"
#include "txtmodel.h"

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

private slots:
    void on_btn_load_received_file_clicked();
    void on_btn_demodulate_clicked();
};
