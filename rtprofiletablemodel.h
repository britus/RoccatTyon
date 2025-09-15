// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once
#include "rtdevicecontroller.h"
#include <QKeyCombination>
#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QtCompare>

/**
 * @brief The RTProfileTableModel class is a proxy between device and UI
 */
class RTProfileTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit RTProfileTableModel(RTDeviceController *device, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private slots:
    void onDeviceFound();
    void onDeviceRemoved();

private:
    RTDeviceController *m_device;
};
