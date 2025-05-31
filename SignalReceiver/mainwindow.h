#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "networkmodel.h"

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

private:
    Ui::MainWindowClass *ui;
    NetworkModel *network_model;
};
