#include "rtcalibratetcudialog.h"
#include "ui_rtcalibratetcudialog.h"
#include <QMainWindow>
#include <QMessageBox>

#define TCU_MAX_TESTS 40

RTCalibrateTcuDialog::RTCalibrateTcuDialog(RTDeviceController *controller, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTCalibrateTcuDialog)
    , m_ctlr(controller)
    , m_timer()
    , m_image()
    , m_sensor()
    , m_dcu(TYON_DISTANCE_CONTROL_UNIT_OFF)
    , m_tcu(TYON_TRACKING_CONTROL_UNIT_OFF)
    , m_median(0)
    , m_imageChanged(false)
    , m_hasError(false)
    , m_count(0)
{
    ui->setupUi(this);

    //| Qt::WindowStaysOnTopHint| Qt::FramelessWindowHint| Qt::CustomizeWindowHint
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(Qt::Tool);

    ui->swWizzard->setCurrentIndex(0);
    ui->swWizzard->setCurrentIndex(0);
    ui->pbApply->setVisible(false);
    ui->pbCancel->setDefault(false);
    ui->pbNextPage->setDefault(true);
    ui->pbNextPage->setFocus();

    ui->tcuImage->setPixelColor(QColor::fromRgb(75, 168, 247));
    ui->progressBar->setVisible(false);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(TCU_MAX_TESTS);
    ui->progressBar->setValue(0);

    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(300);

    m_median = m_ctlr->tcuMedian();
    m_dcu = m_ctlr->dcuState();
    m_tcu = m_ctlr->tcuState();

    connect(&m_timer, &QTimer::timeout, this, &RTCalibrateTcuDialog::onTimer);
    connect(m_ctlr, &RTDeviceController::deviceError, this, &RTCalibrateTcuDialog::onDeviceError, Qt::DirectConnection);
    connect(m_ctlr, &RTDeviceController::sensorChanged, this, &RTCalibrateTcuDialog::onSensorChanged, Qt::DirectConnection);
    connect(m_ctlr, &RTDeviceController::sensorImageChanged, this, &RTCalibrateTcuDialog::onSensorImageChanged, Qt::DirectConnection);

    connect(ui->pbNextPage, &QPushButton::clicked, this, [this, parent]() { //
        ui->swWizzard->setCurrentIndex(1);
        ui->pbNextPage->setVisible(false);
        ui->progressBar->setVisible(true);
        setParentEnabled(parent, false);
        m_timer.start();
    });

    connect(ui->pbCancel, &QPushButton::clicked, this, [this, parent]() { //
        setParentEnabled(parent, true);
        if (!m_isSaved) {
            m_ctlr->tcuSensorCancel(m_dcu);
            reject();
        } else {
            accept();
        }
    });

    connect(ui->pbApply, &QPushButton::clicked, this, [this, parent]() { //
        const QString title = tr("Save calibration");
        const QString msg = tr("Do you want to apply the calibration to the device?\n");

        ui->pbApply->setVisible(false);

        m_ctlr->tcuSensorTest(m_dcu, m_median);

        if (QMessageBox::question(this, title, msg) == QMessageBox::Yes) {
            ui->txInstruction->setText(tr("Well done! Click 'Close' button."));
            if (!m_hasError) {
                m_ctlr->tcuSensorAccept(m_dcu, m_median);
                m_isSaved = !m_hasError;
            }
        } else {
            ui->txInstruction->setText(tr("TCU calibration not saved! Click 'Close' button."));
            m_isSaved = false;
        }

        setParentEnabled(parent, true);
        ui->tcuImage->setVisible(false);
        ui->progressBar->setVisible(false);
        ui->pbCancel->setText(tr("Close"));
        ui->pbCancel->setDefault(true);
    });
}

RTCalibrateTcuDialog::~RTCalibrateTcuDialog()
{
    delete ui;
}

inline void RTCalibrateTcuDialog::setParentEnabled(QWidget *parent, bool enable)
{
    QMainWindow *mw;
    if ((mw = dynamic_cast<QMainWindow *>(parent))) {
        mw->centralWidget()->setEnabled(enable);
        return;
    }
    QWidget *w;
    if ((w = dynamic_cast<QWidget *>(parent))) {
        w->setEnabled(enable);
        return;
    }
}

void RTCalibrateTcuDialog::onDeviceError(int error, const QString &message)
{
    if (m_timer.isActive()) {
        m_timer.stop();
    }
    ui->txInstruction->setText(tr("ERROR %1: %2").arg(error, 8, 16, QChar('0')).arg(message));
    m_hasError = true;
}

void RTCalibrateTcuDialog::onSensorChanged(const TyonSensor &sensor)
{
#ifdef QT_DEBUG
    qDebug("[TCUCAL] onSensorChanged: reg=%d action=%d value=%d", sensor.reg, sensor.action, sensor.value);
#endif
    m_sensor = sensor;
}

void RTCalibrateTcuDialog::onSensorImageChanged(const TyonSensorImage &image)
{
#ifdef QT_DEBUG
    const int size = TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE;
    QByteArray d((char *) image.data, size);
    qDebug("[TCUCAL] onSensorImageChanged: action=%d unused1=%d | image=%s", image.action, image.unused1, qPrintable(d.toHex(' ')));
#endif
    m_imageChanged = true;
    m_image = image;
}

void RTCalibrateTcuDialog::onSensorMedianChanged(int median)
{
#ifdef QT_DEBUG
    qDebug("[TCUCAL] onSensorMedianChanged: median=%d", median);
#endif
    m_median = median;
}

void RTCalibrateTcuDialog::onTimer()
{
    const int imgSize = TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE;

    if (m_hasError) {
        return;
    }

    m_ctlr->tcuSensorCaptureImage();
    m_ctlr->tcuSensorReadImage();

    ++m_count;
    ui->progressBar->setValue(m_count);

    m_imageChanged = false;
    QVector<quint8> data = QVector<quint8>();
    for (int i = 0; i < imgSize; i++) {
        data.append(m_image.data[i]);
    }
    ui->tcuImage->setImageData(data, imgSize);

    m_median = m_ctlr->tcuSensorReadMedian(&m_image);

    ++m_count;
    ui->progressBar->setValue(m_count);

    if (m_count >= TCU_MAX_TESTS) {
        ui->txInstruction->setText( //
            tr("Calibration complete. Press button 'Apply' to save the "
               "calibration or press 'Close' to leave unchanged."));
        ui->progressBar->setVisible(false);
        ui->pbApply->setVisible(true);
        ui->pbApply->setDefault(true);
        ui->pbApply->setFocus();
        m_timer.stop();
    }
}
