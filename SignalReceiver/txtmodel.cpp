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
    txt_received_data_ = in.readAll();
    file.close();
    // 保存为接收到的调制数据
    const auto value_strings = txt_received_data_.split(' ', Qt::SkipEmptyParts);
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
        // 检测振幅变化解调
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
        // 计算相关性解调
        // 按每位数据处理
        for (auto i{ 0 }; i < txt_modulated_data_.size(); i += kSamplesPerBit) {
            double correlation{ 0.0 };
            // 计算每比特占用的采样点数与载波间的相关性
            for (auto j{ 0 }; j < kSamplesPerBit && (i + j) < txt_modulated_data_.size(); ++j) {
                double t = static_cast<double>(j) / kSampleRate;
                double reference_signal = sin(2 * M_PI * kCarrierFreq * t);
                correlation += txt_modulated_data_[i + j] * reference_signal;
            }
            // 通过相关性判断比特值
            // 与载波反相表示1，同相表示0
            txt_demodulated_data_.append(correlation < 0 ? 1 : 0);
        }
    }
}

void TxtModel::DecodeTxtFile(const QString &decode_t)
{
    QByteArray decoded_bytes;
    // 将比特位重组成字节
    for (auto i{ 0 }; i < txt_demodulated_data_.size(); i += 8) {
        if (i + 7 >= txt_demodulated_data_.size()) {
            break;  // 跳过不完整字节
        }
        uint8_t byte{ 0 };
        for (auto j{ 0 }; j < 8; ++j) {
            byte |= (txt_demodulated_data_[i + j] << (7 - j));
        }
        decoded_bytes.append(static_cast<char>(byte));
    }
    // 根据编码类型解码文本
    if (decode_t.compare("UTF-8", Qt::CaseInsensitive) == 0) {
        txt_recovered_data_ = QString::fromUtf8(decoded_bytes);
    } else if (decode_t.compare("UTF-16", Qt::CaseInsensitive) == 0) {
        txt_recovered_data_ = QString::fromUtf16(
            reinterpret_cast<const char16_t *>(decoded_bytes.constData()),
            decoded_bytes.size() / 2
        );
    }
}

void TxtModel::SaveRecoverdFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return;
    }
    QTextStream out(&file);
    out << txt_recovered_data_;
    file.close();
}
