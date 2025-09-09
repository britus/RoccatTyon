#include "rtcolordialog.h"
#include "ui_rtcolordialog.h"
#include <rttypes.h>
#include <QPushButton>

RTColorDialog::RTColorDialog(const RTHidDevice::TDeviceColors &colors, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTColorDialog)
{
    qRegisterMetaType<TyonLight>();

    // --
    ui->setupUi(this);

    //pbColor01
    QPushButton *pb;
    for (quint8 i = 0; i < TYON_LIGHT_INFO_COLORS_NUM; i++) {
        const TyonLight tl = colors[i].deviceColors;
        QString name = tr("pbColor%1").arg(i + 1, 2, 10, QChar('0'));
        if ((pb = ui->pnlColors->findChild<QPushButton *>(name))) {
            pb->setProperty("tli", QVariant::fromValue(tl));
            pb->setBackgroundRole(QPalette::ColorRole::NoRole);
            QString color = tr("background-color: rgb(%1,%2,%3);") //
                                .arg(tl.red)
                                .arg(tl.green)
                                .arg(tl.blue);
            pb->setStyleSheet(color);
            connect(pb, &QPushButton::clicked, this, [this, tl]() { //
                m_selected = tl;
                accept();
            });
        }
    }
}

RTColorDialog::~RTColorDialog()
{
    delete ui;
}
