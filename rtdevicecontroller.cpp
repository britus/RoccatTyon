#include "rtdevicecontroller.h"
#include "rttypes.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

RTDeviceController::RTDeviceController(QObject *parent)
    : QAbstractItemModel(parent)
    , m_device(this)
    , m_buttonTypes()
    , m_physButtons()
{
    initButtonTypes();
    initPhysicalButtons();

    Qt::ConnectionType ct = Qt::QueuedConnection;
    connect(&m_device, &RTHidDevice::lookupStarted, this, &RTDeviceController::onLookupStarted, ct);
    connect(&m_device, &RTHidDevice::deviceWorkerStarted, this, &RTDeviceController::onDeviceWorkerStarted, ct);
    connect(&m_device, &RTHidDevice::deviceWorkerFinished, this, &RTDeviceController::onDeviceWorkerFinished, ct);
    connect(&m_device, &RTHidDevice::deviceError, this, &RTDeviceController::onDeviceError, ct);
    connect(&m_device, &RTHidDevice::deviceFound, this, &RTDeviceController::onDeviceFound, ct);
    connect(&m_device, &RTHidDevice::deviceRemoved, this, &RTDeviceController::onDeviceRemoved, ct);
    connect(&m_device, &RTHidDevice::deviceInfo, this, &RTDeviceController::onDeviceInfo, ct);
    connect(&m_device, &RTHidDevice::profileChanged, this, &RTDeviceController::onProfileChanged, ct);
    connect(&m_device, &RTHidDevice::profileIndexChanged, this, &RTDeviceController::onProfileIndexChanged, ct);
    connect(&m_device, &RTHidDevice::controlUnitChanged, this, &RTDeviceController::onControlUnitChanged, ct);
    connect(&m_device, &RTHidDevice::sensorChanged, this, &RTDeviceController::onSensorChanged, ct);
    connect(&m_device, &RTHidDevice::sensorImageChanged, this, &RTDeviceController::onSensorImageChanged, ct);
}

inline void RTDeviceController::initPhysicalButtons()
{
    TPhysicalButton button_list[TYON_PHYSICAL_BUTTON_NUM] = {
        {QStringLiteral("Left"), TYON_BUTTON_INDEX_LEFT, TYON_BUTTON_INDEX_SHIFT_LEFT},
        {QStringLiteral("Right"), TYON_BUTTON_INDEX_RIGHT, TYON_BUTTON_INDEX_SHIFT_RIGHT},
        {QStringLiteral("Middle"), TYON_BUTTON_INDEX_MIDDLE, TYON_BUTTON_INDEX_SHIFT_MIDDLE},
        {QStringLiteral("Thumb backward"), TYON_BUTTON_INDEX_THUMB_BACK, TYON_BUTTON_INDEX_SHIFT_THUMB_BACK},
        {QStringLiteral("Thumb forward"), TYON_BUTTON_INDEX_THUMB_FORWARD, TYON_BUTTON_INDEX_SHIFT_THUMB_FORWARD},
        {QStringLiteral("Thumb pedal"), TYON_BUTTON_INDEX_THUMB_PEDAL, TYON_BUTTON_INDEX_SHIFT_THUMB_PEDAL},
        {QStringLiteral("X-Celerator up"), TYON_BUTTON_INDEX_THUMB_PADDLE_UP, TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_UP},
        {QStringLiteral("X-Celerator down"), TYON_BUTTON_INDEX_THUMB_PADDLE_DOWN, TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_DOWN},
        {QStringLiteral("Left backward"), TYON_BUTTON_INDEX_LEFT_BACK, TYON_BUTTON_INDEX_SHIFT_LEFT_BACK},
        {QStringLiteral("Left forward"), TYON_BUTTON_INDEX_LEFT_FORWARD, TYON_BUTTON_INDEX_SHIFT_LEFT_FORWARD},
        {QStringLiteral("Right backward"), TYON_BUTTON_INDEX_RIGHT_BACK, TYON_BUTTON_INDEX_SHIFT_RIGHT_BACK},
        {QStringLiteral("Right forward"), TYON_BUTTON_INDEX_RIGHT_FORWARD, TYON_BUTTON_INDEX_SHIFT_RIGHT_FORWARD},
        {QStringLiteral("Fin right"), TYON_BUTTON_INDEX_FIN_RIGHT, TYON_BUTTON_INDEX_SHIFT_FIN_RIGHT},
        {QStringLiteral("Fin left"), TYON_BUTTON_INDEX_FIN_LEFT, TYON_BUTTON_INDEX_SHIFT_FIN_LEFT},
        {QStringLiteral("Wheel up"), TYON_BUTTON_INDEX_WHEEL_UP, TYON_BUTTON_INDEX_SHIFT_WHEEL_UP},
        {QStringLiteral("Wheel down"), TYON_BUTTON_INDEX_WHEEL_DOWN, TYON_BUTTON_INDEX_SHIFT_WHEEL_DOWN},
    };
    for (quint8 i = 0; i < TYON_PHYSICAL_BUTTON_NUM; i++) {
        setPhysicalButton(i, button_list[i]);
    }
}

inline void RTDeviceController::initButtonTypes()
{
    setButtonType(tr("Click"), TYON_BUTTON_TYPE_CLICK);
    setButtonType(tr("Universal scrolling"), TYON_BUTTON_TYPE_UNIVERSAL_SCROLLING);
    setButtonType(tr("Context menu"), TYON_BUTTON_TYPE_MENU);
    setButtonType(tr("Scroll up"), TYON_BUTTON_TYPE_SCROLL_UP);
    setButtonType(tr("Scroll down"), TYON_BUTTON_TYPE_SCROLL_DOWN);
    setButtonType(tr("Tilt left"), TYON_BUTTON_TYPE_TILT_LEFT);
    setButtonType(tr("Tilt right"), TYON_BUTTON_TYPE_TILT_RIGHT);
    setButtonType(tr("Backward"), TYON_BUTTON_TYPE_BROWSER_BACKWARD);
    setButtonType(tr("Forward"), TYON_BUTTON_TYPE_BROWSER_FORWARD);
    setButtonType(tr("Double click"), TYON_BUTTON_TYPE_DOUBLE_CLICK);

    setButtonType(tr("DPI cycle"), TYON_BUTTON_TYPE_CPI_CYCLE);
    setButtonType(tr("DPI up"), TYON_BUTTON_TYPE_CPI_UP);
    setButtonType(tr("DPI down"), TYON_BUTTON_TYPE_CPI_DOWN);

    setButtonType(tr("Open player"), TYON_BUTTON_TYPE_OPEN_PLAYER);
    setButtonType(tr("Previous track"), TYON_BUTTON_TYPE_PREV_TRACK);
    setButtonType(tr("Next track"), TYON_BUTTON_TYPE_NEXT_TRACK);
    setButtonType(tr("Play/Pause"), TYON_BUTTON_TYPE_PLAY_PAUSE);
    setButtonType(tr("Stop"), TYON_BUTTON_TYPE_STOP);
    setButtonType(tr("Mute"), TYON_BUTTON_TYPE_MUTE);
    setButtonType(tr("Volume up"), TYON_BUTTON_TYPE_VOLUME_UP);
    setButtonType(tr("Volume down"), TYON_BUTTON_TYPE_VOLUME_DOWN);

    setButtonType(tr("Profile cycle"), TYON_BUTTON_TYPE_PROFILE_CYCLE);
    setButtonType(tr("Profile up"), TYON_BUTTON_TYPE_PROFILE_UP);
    setButtonType(tr("Profile down"), TYON_BUTTON_TYPE_PROFILE_DOWN);

    setButtonType(tr("Sensitivity cycle"), TYON_BUTTON_TYPE_SENSITIVITY_CYCLE);
    setButtonType(tr("Sensitivity up"), TYON_BUTTON_TYPE_SENSITIVITY_UP);
    setButtonType(tr("Sensitivity down"), TYON_BUTTON_TYPE_SENSITIVITY_DOWN);

    setButtonType(tr("Easywheel Sensitivity"), TYON_BUTTON_TYPE_EASYWHEEL_SENSITIVITY);
    setButtonType(tr("Easywheel Profile"), TYON_BUTTON_TYPE_EASYWHEEL_PROFILE);
    setButtonType(tr("Easywheel CPI"), TYON_BUTTON_TYPE_EASYWHEEL_CPI);
    setButtonType(tr("Easywheel Volume"), TYON_BUTTON_TYPE_EASYWHEEL_VOLUME);
    setButtonType(tr("Easywheel Alt Tab"), TYON_BUTTON_TYPE_EASYWHEEL_ALT_TAB);
    setButtonType(tr("Easywheel Aero Flip 3D"), TYON_BUTTON_TYPE_EASYWHEEL_AERO_FLIP_3D);

    setButtonType(tr("Easyaim Setting 1"), TYON_BUTTON_TYPE_EASYAIM_1);
    setButtonType(tr("Easyaim Setting 2"), TYON_BUTTON_TYPE_EASYAIM_2);
    setButtonType(tr("Easyaim Setting 3"), TYON_BUTTON_TYPE_EASYAIM_3);
    setButtonType(tr("Easyaim Setting 4"), TYON_BUTTON_TYPE_EASYAIM_4);
    setButtonType(tr("Easyaim Setting 5"), TYON_BUTTON_TYPE_EASYAIM_5);

    setButtonType(tr("Self Easyshift"), TYON_BUTTON_TYPE_EASYSHIFT_SELF);
    setButtonType(tr("Other Easyshift"), TYON_BUTTON_TYPE_EASYSHIFT_OTHER);
    setButtonType(tr("Other Easyshift lock"), TYON_BUTTON_TYPE_EASYSHIFT_LOCK_OTHER);
    setButtonType(tr("Both Easyshift"), TYON_BUTTON_TYPE_EASYSHIFT_ALL);

    setButtonType(tr("Timer start"), TYON_BUTTON_TYPE_TIMER);
    setButtonType(tr("Timer stop"), TYON_BUTTON_TYPE_TIMER_STOP);

    setButtonType(tr("XInput 1"), TYON_BUTTON_TYPE_XINPUT_1);
    setButtonType(tr("XInput 2"), TYON_BUTTON_TYPE_XINPUT_2);
    setButtonType(tr("XInput 3"), TYON_BUTTON_TYPE_XINPUT_3);
    setButtonType(tr("XInput 4"), TYON_BUTTON_TYPE_XINPUT_4);
    setButtonType(tr("XInput 5"), TYON_BUTTON_TYPE_XINPUT_5);
    setButtonType(tr("XInput 6"), TYON_BUTTON_TYPE_XINPUT_6);
    setButtonType(tr("XInput 7"), TYON_BUTTON_TYPE_XINPUT_7);
    setButtonType(tr("XInput 8"), TYON_BUTTON_TYPE_XINPUT_8);
    setButtonType(tr("XInput 9"), TYON_BUTTON_TYPE_XINPUT_9);
    setButtonType(tr("XInput 10"), TYON_BUTTON_TYPE_XINPUT_10);
    setButtonType(tr("XInput Rx up"), TYON_BUTTON_TYPE_XINPUT_RX_UP);
    setButtonType(tr("XInput Rx down"), TYON_BUTTON_TYPE_XINPUT_RX_DOWN);
    setButtonType(tr("XInput Ry up"), TYON_BUTTON_TYPE_XINPUT_RY_UP);
    setButtonType(tr("XInput Ry down"), TYON_BUTTON_TYPE_XINPUT_RY_DOWN);
    setButtonType(tr("XInput X up"), TYON_BUTTON_TYPE_XINPUT_X_UP);
    setButtonType(tr("XInput X down"), TYON_BUTTON_TYPE_XINPUT_X_DOWN);
    setButtonType(tr("XInput Y up"), TYON_BUTTON_TYPE_XINPUT_Y_UP);
    setButtonType(tr("XInput Y down"), TYON_BUTTON_TYPE_XINPUT_Y_DOWN);
    setButtonType(tr("XInput Z up"), TYON_BUTTON_TYPE_XINPUT_Z_UP);
    setButtonType(tr("XInput Z down"), TYON_BUTTON_TYPE_XINPUT_Z_DOWN);

    setButtonType(tr("DInput 1"), TYON_BUTTON_TYPE_DINPUT_1);
    setButtonType(tr("DInput 2"), TYON_BUTTON_TYPE_DINPUT_2);
    setButtonType(tr("DInput 3"), TYON_BUTTON_TYPE_DINPUT_3);
    setButtonType(tr("DInput 4"), TYON_BUTTON_TYPE_DINPUT_4);
    setButtonType(tr("DInput 5"), TYON_BUTTON_TYPE_DINPUT_5);
    setButtonType(tr("DInput 6"), TYON_BUTTON_TYPE_DINPUT_6);
    setButtonType(tr("DInput 7"), TYON_BUTTON_TYPE_DINPUT_7);
    setButtonType(tr("DInput 8"), TYON_BUTTON_TYPE_DINPUT_8);
    setButtonType(tr("DInput 9"), TYON_BUTTON_TYPE_DINPUT_9);
    setButtonType(tr("DInput 10"), TYON_BUTTON_TYPE_DINPUT_10);
    setButtonType(tr("DInput 11"), TYON_BUTTON_TYPE_DINPUT_11);
    setButtonType(tr("DInput 12"), TYON_BUTTON_TYPE_DINPUT_12);
    setButtonType(tr("DInput X up"), TYON_BUTTON_TYPE_DINPUT_X_UP);
    setButtonType(tr("DInput X down"), TYON_BUTTON_TYPE_DINPUT_X_DOWN);
    setButtonType(tr("DInput Y up"), TYON_BUTTON_TYPE_DINPUT_Y_UP);
    setButtonType(tr("DInput Y down"), TYON_BUTTON_TYPE_DINPUT_Y_DOWN);
    setButtonType(tr("DInput Z up"), TYON_BUTTON_TYPE_DINPUT_Z_UP);
    setButtonType(tr("DInput Z down"), TYON_BUTTON_TYPE_DINPUT_Z_DOWN);

    setButtonType(tr("Shortcut"), TYON_BUTTON_TYPE_SHORTCUT);
    setButtonType(tr("Windows key"), TYON_BUTTON_TYPE_WINDOWS_KEY);
    setButtonType(tr("Home"), TYON_BUTTON_TYPE_HOME);
    setButtonType(tr("End"), TYON_BUTTON_TYPE_END);
    setButtonType(tr("Page up"), TYON_BUTTON_TYPE_PAGE_UP);
    setButtonType(tr("Page down"), TYON_BUTTON_TYPE_PAGE_DOWN);
    setButtonType(tr("Left ctrl"), TYON_BUTTON_TYPE_L_CTRL);
    setButtonType(tr("Left alt"), TYON_BUTTON_TYPE_L_ALT);

    setButtonType(tr("Macro"), TYON_BUTTON_TYPE_MACRO);
    setButtonType(tr("Quicklaunch"), TYON_BUTTON_TYPE_QUICKLAUNCH);
    setButtonType(tr("Open driver"), TYON_BUTTON_TYPE_OPEN_DRIVER);

    setButtonType(tr("Unused"), TYON_BUTTON_TYPE_UNUSED);
    setButtonType(tr("Disabled"), TYON_BUTTON_TYPE_DISABLED);
}

inline void RTDeviceController::setButtonType(const QString &name, quint8 type)
{
    m_buttonTypes[type] = name;
}

inline void RTDeviceController::setPhysicalButton(quint8 index, TPhysicalButton pb)
{
    m_physButtons[index] = pb;
}

void RTDeviceController::lookupDevice()
{
    m_device.lookupDevice();
}

int RTDeviceController::assignButton(TyonButtonIndex bi, TyonButtonType bt, const QKeyCombination &kc)
{
    m_device.assignButton(bi, bt, kc);
    return 0;
}

void RTDeviceController::setupButton(const RoccatButton &rb, QPushButton *button)
{
    QKeySequence ks;

    // assign ROCCAT button type to push button
    button->setProperty("type", QVariant::fromValue(rb.type));
    button->setProperty("modifier", QVariant::fromValue(rb.modifier));
    button->setProperty("hiduid", QVariant::fromValue(rb.key));
    button->setText(m_buttonTypes[rb.type]);

    // assign shortcut type
    switch (rb.type) {
        case TYON_BUTTON_TYPE_SHORTCUT: {
            ks = toKeySequence(rb);
            if (!ks.isEmpty()) {
                button->setProperty("shortcut", QVariant::fromValue(ks));
#ifndef Q_OS_MACOS
                button->setText(ks.toString());
#else
                if (ks[0].keyboardModifiers().testFlag(Qt::MetaModifier)) {
                    QString s = ks.toString().replace("Meta", "Cmd");
                    button->setText(s);
                } else {
                    button->setText(ks.toString());
                }
#endif
            }
            break;
        }
    }
}

const QKeySequence RTDeviceController::toKeySequence(const RoccatButton &b) const
{
    return m_device.toKeySequence(b);
}

void RTDeviceController::selectProfile(quint8 profileIndex)
{
    m_device.setActiveProfile(profileIndex);
}

qint16 RTDeviceController::toSensitivityXValue(const TyonProfileSettings *settings) const
{
    return m_device.toSensitivityXValue(settings);
}

qint16 RTDeviceController::toSensitivityYValue(const TyonProfileSettings *settings) const
{
    return m_device.toSensitivityYValue(settings);
}

quint16 RTDeviceController::toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const
{
    return m_device.toDpiLevelValue(settings, index);
}

void RTDeviceController::setXSensitivity(qint16 sensitivity)
{
    m_device.setXSensitivity(sensitivity);
}

void RTDeviceController::setYSensitivity(qint16 sensitivity)
{
    m_device.setYSensitivity(sensitivity);
}

void RTDeviceController::setAdvancedSenitivity(quint8 bit, bool state)
{
    m_device.setAdvancedSenitivity(bit, state);
}

void RTDeviceController::setPollRate(quint8 rate)
{
    m_device.setPollRate(rate);
}

void RTDeviceController::setDpiSlot(quint8 bit, bool state)
{
    m_device.setDpiSlot(bit, state);
}

void RTDeviceController::setActiveDpiSlot(quint8 id)
{
    m_device.setActiveDpiSlot(id);
}

void RTDeviceController::setDpiLevel(quint8 index, quint16 value)
{
    m_device.setDpiLevel(index, value);
}

void RTDeviceController::setLightsEnabled(quint8 flag, bool state)
{
    m_device.setLightsEnabled(flag, state);
}

void RTDeviceController::setLightsEffect(quint8 value)
{
    m_device.setLightsEffect(value);
}

void RTDeviceController::setColorFlow(quint8 value)
{
    m_device.setColorFlow(value);
}

void RTDeviceController::setLightColor(TyonLightType target, const TyonLight &light)
{
    m_device.setLightColor(target, light);
}

QString RTDeviceController::profileName() const
{
    return m_device.profileName();
}

void RTDeviceController::loadProfilesFromFile(const QString &fileName)
{
    m_device.loadProfilesFromFile(fileName);
}

void RTDeviceController::saveProfilesToFile(const QString &fileName)
{
    m_device.saveProfilesToFile(fileName);
}

void RTDeviceController::resetProfiles()
{
    m_device.resetProfiles();
}

void RTDeviceController::saveProfilesToDevice()
{
    m_device.saveProfilesToDevice();
}

TyonLight RTDeviceController::toDeviceColor(TyonLightType target, const QColor &color) const
{
    return m_device.toDeviceColor(target, color);
}

QColor RTDeviceController::toScreenColor(const TyonLight &light, bool isCustomColor) const
{
    return m_device.toScreenColor(light, isCustomColor);
}

quint8 RTDeviceController::minimumXCelerate() const
{
    return m_device.minimumXCelerate();
}

quint8 RTDeviceController::maximumXCelerate() const
{
    return m_device.maximumXCelerate();
}

quint8 RTDeviceController::middleXCelerate() const
{
    return m_device.middleXCelerate();
}

void RTDeviceController::startXCCalibration()
{
    m_device.startXCCalibration();
}

void RTDeviceController::stopXCCalibration()
{
    m_device.stopXCCalibration();
}

void RTDeviceController::onLookupStarted()
{
    emit lookupStarted();
}

void RTDeviceController::onDeviceFound()
{
    beginResetModel();
    emit deviceFound();
    endResetModel();
}

void RTDeviceController::onDeviceRemoved()
{
    beginResetModel();
    emit deviceRemoved();
    endResetModel();
}

void RTDeviceController::onDeviceError(int error, const QString &message)
{
    emit deviceError(error, message);
}

void RTDeviceController::onDeviceInfo(const TyonInfo &info)
{
    emit deviceInfoChanged(info);
}

void RTDeviceController::onDeviceWorkerStarted()
{
    emit deviceWorkerStarted();
}

void RTDeviceController::onDeviceWorkerFinished()
{
    emit deviceWorkerFinished();
}

void RTDeviceController::onProfileIndexChanged(const quint8 pix)
{
    emit profileIndexChanged(pix);
}

void RTDeviceController::onProfileChanged(const RTHidDevice::TProfile &profile)
{
    if (profile.settings.size) {
        emit settingsChanged(profile.settings);
    }
    if (profile.buttons.size) {
        emit buttonsChanged(profile.buttons);
    }
}

void RTDeviceController::onControlUnitChanged(const TyonControlUnit &controlUnit)
{
    emit controlUnitChanged(controlUnit);
}

void RTDeviceController::onSensorChanged(const TyonSensor &sensor)
{
    emit sensorChanged(sensor);
}

void RTDeviceController::onSensorImageChanged(const TyonSensorImage &image)
{
    emit sensorImageChanged(image);
}

QVariant RTDeviceController::headerData(int section, Qt::Orientation orientation, int role) const
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

bool RTDeviceController::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}

QModelIndex RTDeviceController::index(int row, int column, const QModelIndex &) const
{
    return createIndex(row, column);
}

QModelIndex RTDeviceController::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int RTDeviceController::rowCount(const QModelIndex &) const
{
    return m_device.profileCount();
}

int RTDeviceController::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant RTDeviceController::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_device.profileCount()) {
        return QVariant();
    }

    bool found = false;
    RTHidDevice::TProfile p = m_device.profile(index.row(), found);
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

bool RTDeviceController::clearItemData(const QModelIndex &index)
{
    return QAbstractItemModel::clearItemData(index);
}

bool RTDeviceController::setData(const QModelIndex &index, const QVariant &value, int role)
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

    m_device.setProfileName(value.toString(), index.row());
    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags RTDeviceController::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool RTDeviceController::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();
    return true;
}

bool RTDeviceController::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    endInsertColumns();
    return true;
}
