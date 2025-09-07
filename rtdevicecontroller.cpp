#include "rtdevicecontroller.h"
#include "hid_uid.h"
#include "rttypes.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

typedef struct
{
    quint8 uid_key;
    Qt::Key qt_key;
    Qt::KeyboardModifier modifier;
} TUidToQtKeyMap;

static TUidToQtKeyMap uid_2_qtkey[] = {
    {HID_UID_KB_A, Qt::Key_A, Qt::NoModifier},
    {HID_UID_KB_B, Qt::Key_B, Qt::NoModifier},
    {HID_UID_KB_C, Qt::Key_C, Qt::NoModifier},
    {HID_UID_KB_D, Qt::Key_D, Qt::NoModifier},
    {HID_UID_KB_E, Qt::Key_E, Qt::NoModifier},
    {HID_UID_KB_F, Qt::Key_F, Qt::NoModifier},
    {HID_UID_KB_G, Qt::Key_G, Qt::NoModifier},
    {HID_UID_KB_H, Qt::Key_H, Qt::NoModifier},
    {HID_UID_KB_I, Qt::Key_I, Qt::NoModifier},
    {HID_UID_KB_J, Qt::Key_J, Qt::NoModifier},
    {HID_UID_KB_K, Qt::Key_K, Qt::NoModifier},
    {HID_UID_KB_L, Qt::Key_L, Qt::NoModifier},
    {HID_UID_KB_M, Qt::Key_M, Qt::NoModifier},
    {HID_UID_KB_N, Qt::Key_N, Qt::NoModifier},
    {HID_UID_KB_O, Qt::Key_O, Qt::NoModifier},
    {HID_UID_KB_P, Qt::Key_P, Qt::NoModifier},
    {HID_UID_KB_Q, Qt::Key_Q, Qt::NoModifier},
    {HID_UID_KB_R, Qt::Key_R, Qt::NoModifier},
    {HID_UID_KB_S, Qt::Key_S, Qt::NoModifier},
    {HID_UID_KB_T, Qt::Key_T, Qt::NoModifier},
    {HID_UID_KB_U, Qt::Key_U, Qt::NoModifier},
    {HID_UID_KB_V, Qt::Key_V, Qt::NoModifier},
    {HID_UID_KB_W, Qt::Key_W, Qt::NoModifier},
    {HID_UID_KB_X, Qt::Key_X, Qt::NoModifier},
    {HID_UID_KB_Y, Qt::Key_Y, Qt::NoModifier},
    {HID_UID_KB_Z, Qt::Key_Z, Qt::NoModifier},
    {HID_UID_KB_1, Qt::Key_1, Qt::NoModifier},
    {HID_UID_KB_2, Qt::Key_2, Qt::NoModifier},
    {HID_UID_KB_3, Qt::Key_3, Qt::NoModifier},
    {HID_UID_KB_4, Qt::Key_4, Qt::NoModifier},
    {HID_UID_KB_5, Qt::Key_5, Qt::NoModifier},
    {HID_UID_KB_6, Qt::Key_6, Qt::NoModifier},
    {HID_UID_KB_7, Qt::Key_7, Qt::NoModifier},
    {HID_UID_KB_8, Qt::Key_8, Qt::NoModifier},
    {HID_UID_KB_9, Qt::Key_9, Qt::NoModifier},
    {HID_UID_KB_0, Qt::Key_0, Qt::NoModifier},
    {HID_UID_KB_ENTER, Qt::Key_Enter, Qt::NoModifier},
    {HID_UID_KB_ESCAPE, Qt::Key_Escape, Qt::NoModifier},
    {HID_UID_KB_BACKSPACE, Qt::Key_Backspace, Qt::NoModifier},
    {HID_UID_KB_TAB, Qt::Key_Tab, Qt::NoModifier},
    {HID_UID_KB_SPACE, Qt::Key_Space, Qt::NoModifier},
    {HID_UID_KB_CAPS_LOCK, Qt::Key_CapsLock, Qt::NoModifier},
    {HID_UID_KB_F1, Qt::Key_F1, Qt::NoModifier},
    {HID_UID_KB_F2, Qt::Key_F2, Qt::NoModifier},
    {HID_UID_KB_F3, Qt::Key_F3, Qt::NoModifier},
    {HID_UID_KB_F4, Qt::Key_F4, Qt::NoModifier},
    {HID_UID_KB_F5, Qt::Key_F5, Qt::NoModifier},
    {HID_UID_KB_F6, Qt::Key_F6, Qt::NoModifier},
    {HID_UID_KB_F7, Qt::Key_F7, Qt::NoModifier},
    {HID_UID_KB_F8, Qt::Key_F8, Qt::NoModifier},
    {HID_UID_KB_F9, Qt::Key_F9, Qt::NoModifier},
    {HID_UID_KB_F10, Qt::Key_F10, Qt::NoModifier},
    {HID_UID_KB_F11, Qt::Key_F11, Qt::NoModifier},
    {HID_UID_KB_F12, Qt::Key_F12, Qt::NoModifier},
    {HID_UID_KB_PRINT_SCREEN, Qt::Key_Print, Qt::NoModifier},
    {HID_UID_KB_SCROLL_LOCK, Qt::Key_ScrollLock, Qt::NoModifier},
    {HID_UID_KB_PAUSE, Qt::Key_Pause, Qt::NoModifier},
    {HID_UID_KB_INSERT, Qt::Key_Insert, Qt::NoModifier},
    {HID_UID_KB_HOME, Qt::Key_Home, Qt::NoModifier},
    {HID_UID_KB_PAGE_UP, Qt::Key_PageUp, Qt::NoModifier},
    {HID_UID_KB_DELETE, Qt::Key_Delete, Qt::NoModifier},
    {HID_UID_KB_END, Qt::Key_End, Qt::NoModifier},
    {HID_UID_KB_PAGE_DOWN, Qt::Key_PageDown, Qt::NoModifier},
    {HID_UID_KB_RIGHT_ARROW, Qt::Key_Right, Qt::NoModifier},
    {HID_UID_KB_LEFT_ARROW, Qt::Key_Left, Qt::NoModifier},
    {HID_UID_KB_DOWN_ARROW, Qt::Key_Down, Qt::NoModifier},
    {HID_UID_KB_UP_ARROW, Qt::Key_Up, Qt::NoModifier},
    {HID_UID_KB_APPLICATION, Qt::Key_Open, Qt::NoModifier},
    {HID_UID_KB_LEFT_CONTROL, Qt::Key_Control, Qt::NoModifier},
    {HID_UID_KB_LEFT_SHIFT, Qt::Key_Shift, Qt::NoModifier},
    {HID_UID_KB_LEFT_ALT, Qt::Key_Alt, Qt::NoModifier},
    {HID_UID_KB_LEFT_GUI, Qt::Key_AltGr, Qt::NoModifier},
    {HID_UID_KB_RIGHT_CONTROL, Qt::Key_Control, Qt::NoModifier},
    {HID_UID_KB_RIGHT_SHIFT, Qt::Key_Shift, Qt::NoModifier},
    {HID_UID_KB_RIGHT_ALT, Qt::Key_Alt, Qt::NoModifier},
    {HID_UID_KB_RIGHT_GUI, Qt::Key_AltGr, Qt::NoModifier},
    {HID_UID_KP_NUM_LOCK, Qt::Key_NumLock, Qt::KeypadModifier},
    {HID_UID_KP_DIV, Qt::Key_division, Qt::KeypadModifier},
    {HID_UID_KP_MUL, Qt::Key_multiply, Qt::KeypadModifier},
    {HID_UID_KP_MINUS, Qt::Key_Minus, Qt::KeypadModifier},
    {HID_UID_KP_PLUS, Qt::Key_Plus, Qt::KeypadModifier},
    {HID_UID_KP_ENTER, Qt::Key_Enter, Qt::KeypadModifier},
    {HID_UID_KP_1, Qt::Key_1, Qt::KeypadModifier},
    {HID_UID_KP_2, Qt::Key_2, Qt::KeypadModifier},
    {HID_UID_KP_3, Qt::Key_3, Qt::KeypadModifier},
    {HID_UID_KP_4, Qt::Key_4, Qt::KeypadModifier},
    {HID_UID_KP_5, Qt::Key_5, Qt::KeypadModifier},
    {HID_UID_KP_6, Qt::Key_6, Qt::KeypadModifier},
    {HID_UID_KP_7, Qt::Key_7, Qt::KeypadModifier},
    {HID_UID_KP_8, Qt::Key_8, Qt::KeypadModifier},
    {HID_UID_KP_9, Qt::Key_9, Qt::KeypadModifier},
    {HID_UID_KP_0, Qt::Key_0, Qt::KeypadModifier},
    {HID_UID_KP_DELETE, Qt::Key_Delete, Qt::KeypadModifier},
    {0, Qt::Key_unknown, Qt::NoModifier},
};

RTDeviceController::RTDeviceController(QObject *parent)
    : QObject(parent)
    , m_device(this)
    , m_model(m_device.profiles())
    , m_buttonTypes()
    , m_physButtons()
    , m_hasDevice(false)
{
    connect(&m_device, &RTHidDevice::lookupStarted, this, &RTDeviceController::onLookupStarted);
    connect(&m_device, &RTHidDevice::deviceError, this, &RTDeviceController::onDeviceError);
    connect(&m_device, &RTHidDevice::deviceFound, this, &RTDeviceController::onDeviceFound);
    connect(&m_device, &RTHidDevice::deviceRemoved, this, &RTDeviceController::onDeviceRemoved);
    connect(&m_device, &RTHidDevice::deviceInfo, this, &RTDeviceController::onDeviceInfo);
    connect(&m_device, &RTHidDevice::profileIndexChanged, this, &RTDeviceController::onProfileIndexChanged);
    connect(&m_device, &RTHidDevice::profileChanged, this, &RTDeviceController::onProfileChanged);
    connect(&m_device, &RTHidDevice::profileChanged, &m_model, &RTProfileModel::onProfileChanged);
    connect(&m_model, &RTProfileModel::dataChanged, this, &RTDeviceController::onModelChanged);

    initButtonTypes();
    initPhysicalButtons();
}

void RTDeviceController::lookupDevice()
{
    m_device.lookupDevice();
}

int RTDeviceController::assignButton( //
    TyonButtonIndex type,
    TyonButtonType function,
    const QKeyCombination &kc)
{
    qDebug() << "[ROCCAT] assignButton:" << type << function << kc;

    if (function != TYON_BUTTON_TYPE_SHORTCUT) {
        m_device.assignButton(type, function, 0, 0);
    } else {
        // Translate QT key modifiers to ROCCAT Tyon modifiers
        auto toQtMods2Roccat = [](const Qt::KeyboardModifiers &km) -> quint8 {
            quint8 mods = 0;
            /* on Mac OSX Meta must be mapped to CTRL */
            if (km.testFlag(Qt::ShiftModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_SHIFT;
            if (km.testFlag(Qt::ControlModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_CTRL;
            if (km.testFlag(Qt::AltModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_ALT;
            if (km.testFlag(Qt::GroupSwitchModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_WIN;
            return mods;
        };

        TUidToQtKeyMap *keymap = nullptr;
        bool isKeyPad = kc.keyboardModifiers().testFlag(Qt::KeypadModifier);
        Qt::KeyboardModifier testMod = (isKeyPad ? Qt::KeypadModifier : Qt::NoModifier);
        for (TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
            if (kc.key() == p->qt_key && p->modifier == testMod) {
                keymap = p;
                break;
            }
        }
        if (!keymap) {
            return EINVAL;
        }

        quint8 mods = toQtMods2Roccat(kc.keyboardModifiers());
        m_device.assignButton(type, function, keymap->uid_key, mods);
    }
    return 0;
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
                button->setText(ks.toString());
            }
            break;
        }
    }
}

const QKeySequence RTDeviceController::toKeySequence(const RoccatButton &b) const
{
    // Translate ROCCAT Tyon key modifier to QT type
    auto toQtModifiers = [](quint8 modifier, TUidToQtKeyMap *keymap) -> Qt::KeyboardModifiers {
        Qt::KeyboardModifiers km = {};
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_SHIFT) {
            km.setFlag(Qt::ShiftModifier, true);
        }
        /* on Mac OSX Ctrl must be mapped to QT 'META' */
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_CTRL) {
            km.setFlag(Qt::ControlModifier, true);
        }
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_ALT) {
            km.setFlag(Qt::AltModifier, true);
        }
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_WIN) {
            km.setFlag(Qt::GroupSwitchModifier, true);
        }
        /* Nummber keypad */
        if (keymap->modifier & Qt::KeypadModifier) {
            km.setFlag(Qt::KeypadModifier, true);
        }
        return km;
    };

    TUidToQtKeyMap *keymap = nullptr;
    for (TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
        if (b.key == p->uid_key) {
            keymap = p;
            break;
        }
    }

    if (keymap) {
        const Qt::KeyboardModifiers km = toQtModifiers(b.modifier, keymap);
        const QKeyCombination kc(km, keymap->qt_key);
        const QKeySequence ks(kc);
        return ks;
    }

    // not found
    return {};
}

void RTDeviceController::selectProfile(quint8 profileIndex)
{
    m_device.setActiveProfile(profileIndex);
}
void RTDeviceController::setXSensitivity(quint8 sensitivity)
{
    m_device.setXSensitivity(sensitivity);
}

void RTDeviceController::setYSensitivity(quint8 sensitivity)
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

void RTDeviceController::setDpiLevel(quint8 index, quint8 value)
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

void RTDeviceController::setLightColorWheel(const QRgb &color)
{
    m_device.setLightColorWheel(qRed(color), qGreen(color), qBlue(color));
}

void RTDeviceController::setLightColorBottom(const QRgb &color)
{
    m_device.setLightColorBottom(qRed(color), qGreen(color), qBlue(color));
}

QString RTDeviceController::profileName() const
{
    return m_device.profileName();
}

bool RTDeviceController::loadProfilesFromFile(const QString &fileName)
{
    return m_device.loadProfilesFromFile(fileName);
}

bool RTDeviceController::saveProfilesToFile(const QString &fileName)
{
    return m_device.saveProfilesToFile(fileName);
}

bool RTDeviceController::resetProfiles()
{
    return m_device.resetProfiles();
}

bool RTDeviceController::saveProfilesToDevice()
{
    return m_device.saveProfilesToDevice();
}

void RTDeviceController::onDeviceFound()
{
    m_hasDevice = m_model.rowCount({}) >= TYON_PROFILE_NUM;
    emit deviceFound();
}

void RTDeviceController::onDeviceRemoved()
{
    m_hasDevice = false;
    m_model.clearItemData({});
    emit deviceRemoved();
}

void RTDeviceController::onLookupStarted()
{
    emit lookupStarted();
}

void RTDeviceController::onDeviceError(int error, const QString &message)
{
    emit deviceError(error, message);
}

void RTDeviceController::onDeviceInfo(const TyonInfo &info)
{
    emit deviceInfoChanged(info);
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

void RTDeviceController::onModelChanged(const QModelIndex &topLeft, //
                                        const QModelIndex &,        //
                                        const QList<int> &)

{
    QVariant v = m_model.data(topLeft, Qt::DisplayRole);
    if (!v.isNull() && v.isValid() && !v.toString().isEmpty()) {
        m_device.setProfileName(v.toString(), topLeft.row());
    }
}
