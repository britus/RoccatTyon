// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtprofiletablemodel.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

RTProfileTableModel::RTProfileTableModel(RTDeviceController *device, QObject *parent)
    : QAbstractItemModel(parent)
    , m_device(device)
{
    Qt::ConnectionType ct = Qt::QueuedConnection;
    connect(m_device, &RTDeviceController::deviceFound, this, &RTProfileTableModel::onDeviceFound, ct);
    connect(m_device, &RTDeviceController::deviceRemoved, this, &RTProfileTableModel::onDeviceRemoved, ct);
}

void RTProfileTableModel::onDeviceFound()
{
    beginResetModel();
    endResetModel();
}

void RTProfileTableModel::onDeviceRemoved()
{
    beginResetModel();
    endResetModel();
}

QVariant RTProfileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: {
                return QVariant(tr("Profiles"));
            }
        }
    }
    return QVariant();
}

QModelIndex RTProfileTableModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex RTProfileTableModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int RTProfileTableModel::rowCount(const QModelIndex &) const
{
    return m_device->profileCount();
}

int RTProfileTableModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant RTProfileTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_device->profileCount()) {
        return QVariant();
    }

    bool found = false;
    RTDeviceController::TProfile p = m_device->profile(index.row(), found);
    if (!found) {
        return QVariant();
    }

    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case 0: {
                    return p.name;
                }
            }
            break;
        }
        case Qt::EditRole: //editting
        case Qt::UserRole: {
            return QVariant::fromValue(p);
        }
    }
    return QVariant();
}

bool RTProfileTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    const QVariant v = data(index, role);
    if (v.isNull() && !v.isValid()) {
        return false;
    }
    if (value.toString().isEmpty()) {
        return false;
    }
    if (value.toString() == v.toString()) {
        return false;
    }

    m_device->setProfileName(value.toString(), index.row());
    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags RTProfileTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}
