#pragma once

#include "rtdevicecontroller.h"
#include "rttypes.h"
#include <QDialog>

namespace Ui {
class RTCalibrateXCDialog;
}

class RTCalibrateXCDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTCalibrateXCDialog(RTDeviceController *controller, QWidget *parent = nullptr);
    ~RTCalibrateXCDialog();

private slots:
    void onDeviceError(int error, const QString &message);
    void onSpecialReport(uint reportId, const QByteArray &report);

private:
    Ui::RTCalibrateXCDialog *ui;
    RTDeviceController *m_ctlr;
    quint8 m_last_value;
    quint8 m_min;
    quint8 m_max;
    quint8 m_mid;
};
