#include "rtprofilemodel.h"

RTProfileModel::RTProfileModel(const RTHidDevice::TProfiles &profiles, QObject *parent)
    : QAbstractItemModel(parent)
    , m_profiles(profiles)
{}

QVariant RTProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
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

bool RTProfileModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}

QModelIndex RTProfileModel::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex RTProfileModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int RTProfileModel::rowCount(const QModelIndex &) const
{
    return m_profiles.count();
}

int RTProfileModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant RTProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_profiles.count()) {
        return QVariant();
    }

    if (!m_profiles.contains(index.row())) {
        return QVariant();
    }

    RTHidDevice::TProfile p = m_profiles[index.row()];

    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case 0: {
                    return p.name;
                }
            }
            break;
        }
        case Qt::UserRole: {
            return QVariant::fromValue(p);
        }
    }
    return QVariant();
}

bool RTProfileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_profiles.count()) {
        return false;
    }
    if (!m_profiles.contains(index.row())) {
        return false;
    }
    if (data(index, role) != value) {
        RTHidDevice::TProfile p = m_profiles[index.row()];
        p.name = value.toString();
        m_profiles[index.row()] = p;
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

Qt::ItemFlags RTProfileModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable; // FIXME: Implement me!
}

bool RTProfileModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();
    return true;
}

bool RTProfileModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    endInsertColumns();
    return true;
}

void RTProfileModel::onProfileChanged(const RTHidDevice::TProfile &p)
{
    if (p.settings.size) {
        // add profile if not exist
        if (!m_profiles.contains(p.index)) {
            beginResetModel();
            m_profiles[p.index] = p;
            endResetModel();
        }
        // check changes
        else {
            const RTHidDevice::TProfile p2 = m_profiles[p.index];
            if (p2.name != p.name || p2.index != p.index) {
                beginResetModel();
                m_profiles[p.index] = p;
                endResetModel();
            }
        }
    }
}
