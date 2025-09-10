#pragma once

#include "rtdevicecontroller.h"
#include <QDialog>

namespace Ui {
class RTCalibrateTcuDialog;
}

class RTCalibrateTcuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTCalibrateTcuDialog(RTDeviceController *controller, QWidget *parent = nullptr);
    ~RTCalibrateTcuDialog();

private:
    Ui::RTCalibrateTcuDialog *ui;
    RTDeviceController *m_ctlr;
};
