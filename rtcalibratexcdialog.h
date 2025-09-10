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
    void onControlUnit(const TyonControlUnit &unit);
    void onDeviceError(int error, const QString &message);

private:
    Ui::RTCalibrateXCDialog *ui;
    RTDeviceController *m_ctlr;
};
