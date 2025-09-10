#include "rtcalibratexcdialog.h"
#include "rttypes.h"
#include "ui_rtcalibratexcdialog.h"
#include <QPushButton>

RTCalibrateXCDialog::RTCalibrateXCDialog(RTDeviceController *controller, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTCalibrateXCDialog)
    , m_ctlr(controller)
    , m_last_value(0)
    , m_min(0)
    , m_max(0)
    , m_mid(0)
{
    ui->setupUi(this);
    ui->swWizzard->setCurrentIndex(0);
    ui->pbPrevPage->setVisible(false);
    ui->pbApply->setVisible(false);

    //| Qt::WindowStaysOnTopHint
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    connect(m_ctlr, &RTDeviceController::deviceError, this, &RTCalibrateXCDialog::onDeviceError, Qt::DirectConnection);
    connect(m_ctlr, &RTDeviceController::specialReport, this, &RTCalibrateXCDialog::onSpecialReport, Qt::DirectConnection);

    //quint8 min = m_ctlr->minimumXCelerate();
    //quint8 mid = m_ctlr->middleXCelerate();
    //quint8 max = m_ctlr->maximumXCelerate();

    ui->vslXCelerate->setEnabled(false);
    ui->vslXCelerate->setMinimum(m_min);
    ui->vslXCelerate->setMaximum(m_max);
    ui->vslXCelerate->setValue(m_mid);

    connect(ui->pbCancel, &QPushButton::clicked, this, [this]() { //
        m_ctlr->xcStopCalibration();
        this->reject();
    });

    connect(ui->pbApply, &QPushButton::clicked, this, [this]() { //
        m_ctlr->xcApplyCalibration(m_min, m_mid, m_max);
        this->reject();
    });

    connect(ui->pbNextPage, &QPushButton::clicked, this, [this]() { //
        ui->swWizzard->setCurrentIndex(1);
        ui->pbNextPage->setEnabled(false);
        m_ctlr->xcStartCalibration();
    });
}

RTCalibrateXCDialog::~RTCalibrateXCDialog()
{
    m_ctlr->disconnect(this);
    delete ui;
}

void RTCalibrateXCDialog::onDeviceError(int error, const QString &message)
{
    ui->txInstruction->setText(tr("ERROR %1: %2").arg(error, 8, 16, QChar('0')).arg(message));
}

void RTCalibrateXCDialog::onSpecialReport(uint reportId, const QByteArray &report)
{
    TyonSpecial *p = (TyonSpecial *) report.constData();

    // sane check
    if ((ulong) report.length() < sizeof(TyonSpecial)) {
        return;
    }
    // 0x03
    if (reportId != TYON_REPORT_ID_SPECIAL) {
        return;
    }
    // 0xE0
    if (p->type != TYON_SPECIAL_TYPE_XCELERATOR_CALIBRATION) {
        return;
    }

    qDebug("[XC-CAL] irid=%d rid=0x%02x type=%d  analogue=%d data=%d action=%d pl=%s", //
           reportId,
           p->report_id,
           p->type,
           p->analogue,
           p->data,
           p->action,
           qPrintable(report.toHex(' ')));

    if (p->action != m_last_value) {
        m_last_value = p->action;

        if (p->action < ui->vslXCelerate->minimum()) {
            ui->vslXCelerate->setMinimum(p->action);
        }

        if (p->action > ui->vslXCelerate->maximum()) {
            ui->vslXCelerate->setMaximum(p->action);
        }

        ui->vslXCelerate->setValue(p->action);
    }
}
