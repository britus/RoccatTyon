// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once

#include "rthiddevice.h"
#include "rttypedefs.h"
#include <QDialog>

namespace Ui {
class RTColorDialog;
}

class RTColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTColorDialog(const RTHidDevice::TDeviceColors &colors, QWidget *parent = nullptr);
    ~RTColorDialog();
    inline const TyonLight &selectedColor() const { return m_selected; }

private:
    Ui::RTColorDialog *ui;
    TyonLight m_selected;
};
