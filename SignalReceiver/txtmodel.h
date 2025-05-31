#pragma once

#include <QObject>
#include <QList>

class TxtModel  : public QObject
{
    Q_OBJECT

public:
    TxtModel(QObject *parent);
    ~TxtModel();

    bool LoadTxtFile(const QString &file_name);
    void DemodulateTxtFile(const QString &demodulate_t);
    void DecodeTxtFile(const QString &decode_t);
    void SaveRecoverdFile(const QString &file_name);

    const QString &get_txt_received_data() const { return txt_received_data_; }
    const QList<double> &get_txt_modulated_data() const { return txt_modulated_data_; }
    const QList<uint8_t> &get_txt_demodulated_data() const { return txt_demodulated_data_; }
    const QString &get_txt_recovered_data() const { return txt_recovered_data_; }

    static constexpr double kSampleRate{ 1600.0 };
    static constexpr qsizetype kSamplesPerBit{ 16 };
    static constexpr double kCarrierFreq{ 200 };

private:
    QString txt_received_data_;
    QList<double> txt_modulated_data_;
    QList<uint8_t> txt_demodulated_data_;
    QString txt_recovered_data_;
};

