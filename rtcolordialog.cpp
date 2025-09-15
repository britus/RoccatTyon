// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtcolordialog.h"
#include "ui_rtcolordialog.h"
#include <rttypedefs.h>
#include <QPushButton>

RTColorDialog::RTColorDialog(const RTDeviceController::TDeviceColors &colors, QWidget *parent)
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
