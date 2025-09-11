#pragma once

#include "rtdevicecontroller.h"
#include <QDialog>
#include <QTimer>

namespace Ui {
class RTCalibrateTcuDialog;
}

class RTCalibrateTcuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTCalibrateTcuDialog(RTDeviceController *controller, QWidget *parent = nullptr);
    ~RTCalibrateTcuDialog();

private slots:
    void onDeviceError(int error, const QString &message);
    void onSensorChanged(const TyonSensor &sensor);
    void onSensorImageChanged(const TyonSensorImage &image);
    void onSensorMedianChanged(int median);
    void onTimer();

private:
    Ui::RTCalibrateTcuDialog *ui;
    RTDeviceController *m_ctlr;
    QTimer m_timer;
    TyonSensorImage m_image;
    TyonSensor m_sensor;
    TyonControlUnitDcu m_dcu;
    TyonControlUnitTcu m_tcu;
    int m_median;
    bool m_isSaved;
    bool m_imageChanged;
    bool m_hasError;
    int m_count;

private:
    inline void setParentEnabled(QWidget *parent, bool enable = true);
};
