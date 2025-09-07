#include "rtcolordialog.h"
#include "ui_rtcolordialog.h"
#include <rttypes.h>
#include <QPushButton>

//  index, state, red, green, blue, null, checksum;
static TyonRmpLightInfo const roccat_colors[TYON_RMP_LIGHT_INFO_COLORS_NUM] = {
    {0x00, TYON_RMP_LIGHT_INFO_STATE_ON, 0x05, 0x90, 0xfe, 0x00, 0x94},
    {0x01, TYON_RMP_LIGHT_INFO_STATE_ON, 0x00, 0x71, 0xff, 0x00, 0x72},
    {0x02, TYON_RMP_LIGHT_INFO_STATE_ON, 0x00, 0x00, 0xff, 0x00, 0x02},
    {0x03, TYON_RMP_LIGHT_INFO_STATE_ON, 0x5c, 0x18, 0xe6, 0x00, 0x5e},
    {0x04, TYON_RMP_LIGHT_INFO_STATE_ON, 0x81, 0x18, 0xe6, 0x00, 0x84},
    {0x05, TYON_RMP_LIGHT_INFO_STATE_ON, 0xc5, 0x18, 0xe6, 0x00, 0xc9},
    {0x06, TYON_RMP_LIGHT_INFO_STATE_ON, 0xf8, 0x04, 0x7c, 0x00, 0x7f},
    {0x07, TYON_RMP_LIGHT_INFO_STATE_ON, 0xff, 0x00, 0x00, 0x00, 0x07},
    {0x08, TYON_RMP_LIGHT_INFO_STATE_ON, 0xf7, 0x79, 0x00, 0x00, 0x79},
    {0x09, TYON_RMP_LIGHT_INFO_STATE_ON, 0xe7, 0xdc, 0x00, 0x00, 0xcd},
    {0x0a, TYON_RMP_LIGHT_INFO_STATE_ON, 0xc2, 0xf2, 0x08, 0x00, 0xc7},
    {0x0b, TYON_RMP_LIGHT_INFO_STATE_ON, 0x00, 0xff, 0x00, 0x00, 0x0b},
    {0x0c, TYON_RMP_LIGHT_INFO_STATE_ON, 0x18, 0xa6, 0x2a, 0x00, 0xf5},
    {0x0d, TYON_RMP_LIGHT_INFO_STATE_ON, 0x13, 0xec, 0x96, 0x00, 0xa3},
    {0x0e, TYON_RMP_LIGHT_INFO_STATE_ON, 0x0d, 0xe2, 0xd9, 0x00, 0xd7},
    {0x0f, TYON_RMP_LIGHT_INFO_STATE_ON, 0x00, 0xbe, 0xf4, 0x00, 0xc2},
};

Q_DECLARE_METATYPE(TyonRmpLightInfo);

RTColorDialog::RTColorDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTColorDialog)
{
    qRegisterMetaType<TyonRmpLightInfo>();

    // --
    ui->setupUi(this);

    //pbColor01
    QPushButton *pb;
    for (quint8 i = 0; i < TYON_RMP_LIGHT_INFO_COLORS_NUM; i++) {
        TyonRmpLightInfo tli = roccat_colors[i];
        QString name = tr("pbColor%1").arg(i + 1, 2, 10, QChar('0'));
        if ((pb = ui->pnlColors->findChild<QPushButton *>(name))) {
            pb->setProperty("tli", QVariant::fromValue(tli));
            pb->setBackgroundRole(QPalette::ColorRole::NoRole);
            QString color = tr("background-color: rgb(%1,%2,%3);") //
                                .arg(tli.red)
                                .arg(tli.green)
                                .arg(tli.blue);
            pb->setStyleSheet(color);
            connect(pb, &QPushButton::clicked, this, [this, tli]() { //
                m_selected = tli;
                accept();
            });
        }
    }
}

RTColorDialog::~RTColorDialog()
{
    delete ui;
}
