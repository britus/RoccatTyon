// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once

#include "rtdevicecontroller.h"
#include <QDialog>
#include <QTime>

namespace Ui {
class RTCalibrateXCDialog;
}

class RTCalibrateXCDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RTCalibrateXCDialog(RTDeviceController *controller, QWidget *parent = nullptr);
    ~RTCalibrateXCDialog();

private slots:
    void onDeviceError(int error, const QString &message);
    void onSpecialReport(uint reportId, const QByteArray &report);

private:
    Ui::RTCalibrateXCDialog *ui;
    RTDeviceController *m_ctlr;
    QStringList m_instructions;
    bool m_isSaved;
    uint m_stage;
    uint m_count;
    QTime m_phaseStart;
    qint32 m_last_value;
    qint32 m_min;
    qint32 m_max;
    qint32 m_mid;

private:
    inline void setParentEnabled(QWidget *parent, bool enable = true);
    inline bool inRange(qint32 a, qint32 b, qint32 range);
    inline void setPhase(uint phase);
    inline void nextPhase();
};
