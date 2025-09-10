#include "rtcalibratexcdialog.h"
#include "rttypes.h"
#include "ui_rtcalibratexcdialog.h"
#include <QPushButton>

RTCalibrateXCDialog::RTCalibrateXCDialog(RTDeviceController *controller, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTCalibrateXCDialog)
    , m_ctlr(controller)
{
    ui->setupUi(this);
    ui->pbPrevPage->setVisible(false);
    ui->swWizzard->setCurrentIndex(0);

    connect(m_ctlr,
            &RTDeviceController::controlUnitChanged, //
            this,
            &RTCalibrateXCDialog::onControlUnit);
    connect(m_ctlr,
            &RTDeviceController::deviceError, //
            this,
            &RTCalibrateXCDialog::onDeviceError);

    ui->vslXCelerate->setMinimum(m_ctlr->minimumXCelerate());
    ui->vslXCelerate->setMaximum(m_ctlr->maximumXCelerate());
    ui->vslXCelerate->setValue(m_ctlr->middleXCelerate());

    connect(ui->pbCancel, &QPushButton::clicked, this, [this]() { //
        m_ctlr->stopXCCalibration();
        this->reject();
    });

    connect(ui->pbNextPage, &QPushButton::clicked, this, [this]() { //
        m_ctlr->startXCCalibration();
    });
}

RTCalibrateXCDialog::~RTCalibrateXCDialog()
{
    m_ctlr->disconnect(this);
    delete ui;
}

void RTCalibrateXCDialog::onDeviceError(int error, const QString &message)
{
    m_ctlr->stopXCCalibration();
}

void RTCalibrateXCDialog::onControlUnit(const TyonControlUnit &unit)
{
    if (ui->swWizzard->currentIndex() == 0) {
        ui->swWizzard->setCurrentIndex(1);
    }

    //
#if 0
    switch (p->action) {
        case TYON_CONTROL_UNIT_ACTION_ACCEPT: {
            break;
        }
        case TYON_CONTROL_UNIT_ACTION_CANCEL: {
            break;
        }
        case TYON_CONTROL_UNIT_ACTION_OFF: {
            break;
        }
        case TYON_CONTROL_UNIT_ACTION_UNDEFINED: {
            break;
        }
    }
    switch (p->tcu) {
        case TYON_TRACKING_CONTROL_UNIT_ON: {
            break;
        }
        case TYON_TRACKING_CONTROL_UNIT_OFF: {
            break;
        }
        case 0xff: {
            break;
        }
    }
#endif
}
