// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once

#include <QDialog>
#include <QMutex>
#include <QPointer>

class QProgressBar;

class RTProgress : public QDialog
{
    Q_OBJECT

public:
    static void present(const QString &message, QWidget *parent = nullptr);
    static void dismiss();
    static void setProgress(int value); // 0 - 100

protected:
    explicit RTProgress(const QString &message, QWidget *parent = nullptr);

private:
    static QPointer<RTProgress> instance;
    static QMutex mutex;
    QProgressBar *progressBar;
};
