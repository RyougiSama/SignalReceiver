#include "txtmodel.h"
#include <QFile>
#include <QMessageBox>

TxtModel::TxtModel(QObject *parent)
    : QObject(parent)
{}

TxtModel::~TxtModel()
{}

bool TxtModel::LoadTxtFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return false;
    }
    QTextStream in(&file);
    txt_raw_data_ = in.readAll();
    file.close();
    // 保存为接收到的调制数据
    const auto value_strings = txt_raw_data_.split(' ', Qt::SkipEmptyParts);
    txt_modulated_data_.clear();
    txt_modulated_data_.reserve(value_strings.size());
    txt_demodulated_data_.reserve(value_strings.size());
    for (const auto &value_string : value_strings) {
        bool ok{ false };
        auto value = value_string.toDouble(&ok);
        if (ok) {
            txt_modulated_data_.append(value);
        } else {
            QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Invalid data in file: %1")
                                 .arg(value_string));
            return false;
        }
    }
    return true;
}

void TxtModel::DemodulateTxtFile(const QString &demodulate_t)
{
    if (demodulate_t.compare("ASK", Qt::CaseInsensitive) == 0) {
        // 按每位数据处理
        for (auto i{ 0 }; i < txt_modulated_data_.size(); i += kSamplesPerBit) {
            double energy{ 0.0 };
            // 计算每比特占用的采样点数的能量
            for (auto j{ 0 }; j < kSamplesPerBit && (i + j) < txt_modulated_data_.size(); ++j) {
                energy += txt_modulated_data_[i + j] * txt_modulated_data_[i + j];
            }
            // 通过能量阈值判断比特值
            const double threshold{ 0.5 * kSamplesPerBit / 10.0 };
            txt_demodulated_data_.append(energy > threshold ? 1 : 0);
        }
    } else if (demodulate_t.compare("PSK", Qt::CaseInsensitive) == 0) {
    
    }
}
