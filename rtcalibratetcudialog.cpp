#include "rtcalibratetcudialog.h"
#include "ui_rtcalibratetcudialog.h"

RTCalibrateTcuDialog::RTCalibrateTcuDialog(RTDeviceController *controller, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTCalibrateTcuDialog)
    , m_ctlr(controller)
{
    ui->setupUi(this);
    ui->swWizzard->setCurrentIndex(0);
}

RTCalibrateTcuDialog::~RTCalibrateTcuDialog()
{
    delete ui;
}
