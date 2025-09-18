// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtcontroller.h"
#include "hid_uid.h"
#include "rttypedefs.h"
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputMethod>
#include <QKeySequence>
#include <QLocale>
#include <QMutexLocker>
#include <QRgb>
#include <QRgba64>
#include <QRgbaFloat16>
#include <QRgbaFloat32>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

#ifdef Q_OS_MACOS
#include "rthidmacos.h"
#endif
#ifdef Q_OS_LINUX
#include "rthidlinux.h"
#endif

#undef QT_DEBUG

#ifdef QT_DEBUG
#include "rthiddevicedbg.hpp"
#endif

typedef struct
{
    quint8 uid_key;
    Qt::Key qt_key;
    Qt::KeyboardModifier modifier;
} TUidToQtKeyMap;

static TUidToQtKeyMap const uid_2_qtkey[] = {
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

/*
 * Roccat color index table with UI RGB colors. This is a fixed color
 * set from the device. Index is used by the device as reference.
 * Columns: index, red, green, blue, unused
*/
static TyonLight const roccat_colors[TYON_LIGHT_INFO_COLORS_NUM] = {
    {0x00, 0x05, 0x90, 0xfe, 0x94},
    {0x01, 0x00, 0x71, 0xff, 0x72},
    {0x02, 0x00, 0x00, 0xff, 0x02},
    {0x03, 0x5c, 0x18, 0xe6, 0x5e},
    {0x04, 0x81, 0x18, 0xe6, 0x84},
    {0x05, 0xc5, 0x18, 0xe6, 0xc9},
    {0x06, 0xf8, 0x04, 0x7c, 0x7f},
    {0x07, 0xff, 0x00, 0x00, 0x07},
    {0x08, 0xf7, 0x79, 0x00, 0x79},
    {0x09, 0xe7, 0xdc, 0x00, 0xcd},
    {0x0a, 0xc2, 0xf2, 0x08, 0xc7},
    {0x0b, 0x00, 0xff, 0x00, 0x0b},
    {0x0c, 0x18, 0xa6, 0x2a, 0xf5},
    {0x0d, 0x13, 0xec, 0x96, 0xa3},
    {0x0e, 0x0d, 0xe2, 0xd9, 0xd7},
    {0x0f, 0x00, 0xbe, 0xf4, 0xc2},
};

// -------------------------------------------------------------

RTController::RTController(QObject *parent)
    : QObject{parent}
    , m_hid(nullptr)
    , m_handlers()
    , m_colors()
    , m_info()
    , m_activeProfile()
    , m_profiles()
    , m_talkFx()
    , m_sensor()
    , m_sensorImage()
    , m_controlUnit()
    , m_requestedProfile(0)
    , m_initComplete(false)
{
    initButtonTypes();
    initPhysicalButtons();
    initializeColorMapping();
    initializeProfiles();
    initializeHandlers();

#ifdef Q_OS_MACOS
    m_hid = new RTHidMacOS(this);
#endif

#ifdef Q_OS_LINUX
    m_hid = new RTHidLinux(this);
#endif

    Qt::ConnectionType ct = Qt::DirectConnection;
    connect(m_hid, &RTAbstractDevice::lookupStarted, this, &RTController::onLookupStarted, ct);
    connect(m_hid, &RTAbstractDevice::deviceFound, this, &RTController::onDeviceFound, ct);
    connect(m_hid, &RTAbstractDevice::deviceRemoved, this, &RTController::onDeviceRemoved, ct);
    connect(m_hid, &RTAbstractDevice::errorOccured, this, &RTController::onErrorOccured, ct);
    connect(m_hid, &RTAbstractDevice::inputReady, this, &RTController::onInputReady, ct);
    // register HID report handlers
    m_hid->registerHandlers(m_handlers);
}

RTController::~RTController()
{
    internalSaveProfiles();
    if (m_hid) {
        m_hid->disconnect(this);
        delete m_hid;
        m_hid = nullptr;
    }
}

void RTController::onLookupStarted()
{
    emit lookupStarted();
}

void RTController::onDeviceFound(THidDeviceType type)
{
    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, type]() {
        // if misc input device channel, skip
        if (type == THidDeviceType::HidMouseInput) {
            goto func_exit;
        }

        /* read device control state */
        if (!readDeviceControl()) {
            goto func_exit;
        }

        /* read firmware release */
        if (!readDeviceInfo()) {
            goto func_exit;
        }

        /* read device control unit */
        if (!readControlUnit()) {
            goto func_exit;
        }

        /* read talk-fx status */
        if (!talkRead()) {
            goto func_exit;
        }

        /* read current profile number */
        if (!readActiveProfile()) {
            goto func_exit;
        }

        /* read profile slots */
        for (quint8 pix = 0; pix < TYON_PROFILE_NUM; pix++) {
            if (!readProfiles(pix)) {
                goto func_exit;
            }
        }

        emit deviceFound();

    func_exit:
        t->exit(0);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [](QObject *) { //
        //emit deviceWorkerFinished();
    });
    t->start(QThread::IdlePriority);
    QThread::yieldCurrentThread();
}

void RTController::onDeviceRemoved()
{
    emit deviceRemoved();
}

void RTController::onErrorOccured(int error, const QString &message)
{
    raiseError(error, message);
}

void RTController::onInputReady(quint32 rid, const QByteArray &data)
{
    // X-Celerator calibration events
    if (rid == TYON_REPORT_ID_SPECIAL) {
        emit specialReport(rid, data);
    }
}

inline void RTController::initPhysicalButtons()
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

inline void RTController::initButtonTypes()
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

inline void RTController::initializeColorMapping()
{
    for (quint8 i = 0; i < TYON_LIGHT_INFO_COLORS_NUM; i++) {
        m_colors[i].deviceColors = roccat_colors[i];
    }
}

inline void RTController::initializeProfiles()
{
    for (quint8 i = 0; i < TYON_PROFILE_NUM; i++) {
        TProfile profile = {};
        profile.index = i;
        profile.name = tr("Default_%1").arg(profile.index + 1);
        setModified(&profile, true);
        m_profiles[i] = profile;
    }

    QString fpath = QStandardPaths::writableLocation( //
        QStandardPaths::AppConfigLocation);
    fpath = QDir::toNativeSeparators(fpath + "/profiles.rtpf");

    if (QFile::exists(fpath)) {
        loadProfilesFromFile(fpath, false);
    }
}

// At this point it's also possible to change the value type to an
// interface class for product specific HID report handlers
inline void RTController::initializeHandlers()
{
    auto checkLength = [](qsizetype length, qsizetype required) -> bool {
        return (length != required ? false : true);
    };
    m_handlers[TYON_REPORT_ID_CONTROL] = [checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(RoccatControl))) {
            return false;
        }
#ifdef QT_DEBUG
        RoccatControl *p = (RoccatControl *) buffer;
        qDebug("[HIDDEV] TYON_REPORT_ID_CONTROL: reqest=0x%02x value=0x%02x", p->request, p->value);
#endif
        return true;
    };
    m_handlers[TYON_REPORT_ID_INFO] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonInfo))) {
            return false;
        }
        memcpy(&m_info, buffer, sizeof(TyonInfo));
#ifdef QT_DEBUG
        debugDevInfo(&m_info);
#endif
        emit deviceInfo(m_info);
        return true;
    };
    m_handlers[TYON_REPORT_ID_PROFILE] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonProfile))) {
            return false;
        }
        TyonProfile *p = (TyonProfile *) buffer;
#ifdef QT_DEBUG
        qDebug("[HIDDEV] PROFILE: ACTIVE_PROFILE=%d", p->profile_index);
#endif
        memcpy(&m_activeProfile, p, sizeof(TyonProfile));
        TProfile profile = m_profiles[m_activeProfile.profile_index];
        profile.index = p->profile_index;
        m_profiles[profile.index] = profile;
        emit profileIndexChanged(profile.index);
        return true;
    };
    m_handlers[TYON_REPORT_ID_PROFILE_SETTINGS] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonProfileSettings))) {
            return false;
        }
        TyonProfileSettings *p = (TyonProfileSettings *) buffer;
        TProfile profile = m_profiles[p->profile_index];
        profile.index = p->profile_index;
        memcpy(&profile.settings, p, sizeof(TyonProfileSettings));
        m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
        debugSettings(profile, profile.index);
#endif
        emit profileChanged(profile);
        return true;
    };
    m_handlers[TYON_REPORT_ID_PROFILE_BUTTONS] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonProfileButtons))) {
            return false;
        }
        TyonProfileButtons *p = (TyonProfileButtons *) buffer;
        TProfile profile = m_profiles[p->profile_index];
        profile.index = p->profile_index;
        memcpy(&profile.buttons, p, sizeof(TyonProfileButtons));
        m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
        debugButtons(profile, profile.index);
#endif
        emit profileChanged(profile);
        return true;
    };
    // TODO: Implement macro support
    m_handlers[TYON_REPORT_ID_MACRO] = [checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonMacro))) {
            return false;
        }
        TyonMacro *p = (TyonMacro *) buffer;
#ifdef QT_DEBUG
        qDebug("[HIDDEV] MACRO: #%02d group=%s name=%s count=%d", p->button_index, p->macroset_name, p->macro_name, p->count);
#else
        Q_UNUSED(p);
#endif
        return true;
    };
    m_handlers[TYON_REPORT_ID_DEVICE_STATE] = [checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonDeviceState))) {
            return false;
        }
        TyonDeviceState *p = (TyonDeviceState *) buffer;
#ifdef QT_DEBUG
        qDebug("[HIDDEV] DEVICE_STATE: state=%d", p->state);
#endif
        return true;
    };
    m_handlers[TYON_REPORT_ID_CONTROL_UNIT] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonControlUnit))) {
            return false;
        }
        TyonControlUnit *p = (TyonControlUnit *) buffer;
#ifdef QT_DEBUG
        qDebug("[HIDDEV] CONTROL_UNIT: action=0x%02x dcu=%d tcu=%d median=%d", p->action, p->dcu, p->tcu, p->median);
#endif
        memcpy(&m_controlUnit, p, sizeof(TyonControlUnit));
        emit controlUnitChanged(m_controlUnit);
        return true;
    };
    m_handlers[TYON_REPORT_ID_SENSOR] = [this](const quint8 *buffer, qsizetype length) -> bool { //
        TyonSensor *p = (TyonSensor *) buffer;
#ifdef QT_DEBUG
        qDebug("[HIDDEV] SENSOR: action=%d reg=%d value=%d", p->action, p->reg, p->value);
#endif
        if (length == sizeof(TyonSensor)) {
            memcpy(&m_sensor, p, sizeof(TyonSensor));
            emit sensorChanged(m_sensor);
        } else if (length == sizeof(TyonSensorImage)) {
            memcpy(&m_sensorImage, p, sizeof(TyonSensorImage));
            emit sensorImageChanged(m_sensorImage);
        } else {
            return false;
        }
        return true;
    };
    m_handlers[TYON_REPORT_ID_TALK] = [this, checkLength](const quint8 *buffer, qsizetype length) -> bool { //
        if (!checkLength(length, sizeof(TyonTalk))) {
            return false;
        }
        TyonTalk *p = (TyonTalk *) buffer;
#ifdef QT_DEBUG
        debugTalkFx(p);
#endif
        memcpy(&m_talkFx, p, sizeof(TyonTalk));
        emit talkFxChanged(m_talkFx);
        return true;
    };
}

inline void RTController::setButtonType(const QString &name, quint8 type)
{
    m_buttonTypes[type] = name;
}

inline void RTController::setPhysicalButton(quint8 index, TPhysicalButton pb)
{
    m_physButtons[index] = pb;
}

void RTController::lookupDevice()
{
    m_initComplete = false;

    QList<quint32> products;
    products.append(USB_DEVICE_ID_ROCCAT_TYON_BLACK);
    products.append(USB_DEVICE_ID_ROCCAT_TYON_WHITE);

    if (!m_hid->lookupDevices(USB_DEVICE_ID_VENDOR_ROCCAT, products)) {
        raiseError(EIO, "Unable to lookup ROCCAT Tyon device.");
    }
}

void RTController::resetProfiles()
{
    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_RESET;
    quint8 *buffer = (quint8 *) &info;

    m_profiles.clear();
    m_activeProfile = {};
    m_info = {};

    initializeProfiles();

    if (!roccatControlCheck()) {
        return;
    }

    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    if (!m_hid->writeHidMessage(hdt, info.report_id, buffer, info.size)) {
        return;
    }

    // wait reset...
    QThread::msleep(1000);

    // restart device lookup
    lookupDevice();
}

void RTController::updateDevice()
{
    emit deviceWorkerStarted();

    auto writeProfileIndex = [this]() -> bool {
        const THidDeviceType hdt = THidDeviceType::HidMouseControl;
        const qsizetype length = sizeof(TyonProfile);
        const quint8 *buffer = (quint8 *) &m_activeProfile;

        if (!roccatControlCheck()) {
            return false;
        }

        if (!m_hid->writeHidMessage(hdt, TYON_REPORT_ID_PROFILE, buffer, length)) {
            return false;
        }

        return true;
    };

    auto writeProfile = [this](const TProfile &p) -> int {
        const THidDeviceType hdt = THidDeviceType::HidMouseControl;
        qsizetype length;
        quint8 *buffer;

        length = sizeof(TyonProfileSettings);
        buffer = (quint8 *) &p.settings;

        if (!roccatControlCheck()) {
            return false;
        }
        if (!m_hid->writeHidMessage(hdt, TYON_REPORT_ID_PROFILE_SETTINGS, buffer, length)) {
            return false;
        }

        length = sizeof(TyonProfileButtons);
        buffer = (quint8 *) &p.buttons;

        if (!roccatControlCheck()) {
            return false;
        }
        if (!m_hid->writeHidMessage(hdt, TYON_REPORT_ID_PROFILE_BUTTONS, buffer, length)) {
            return false;
        }

        return true;
    };

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, writeProfileIndex, writeProfile]() { //
        /* update TCU / DCU */
        if (m_controlUnit.tcu == TYON_TRACKING_CONTROL_UNIT_OFF) {
            if (!tcuWriteOff(m_controlUnit.dcu)) {
                goto thread_exit;
            }
        } else if (!tcuWriteAccept(m_controlUnit.dcu, m_controlUnit.median)) {
            goto thread_exit;
        }

        /* set active profile */
        if (!writeProfileIndex()) {
            goto thread_exit;
        }

        /* write all profiles */
        foreach (TProfile p, m_profiles) {
            if (p.changed) {
                if (!writeProfile(p)) {
                    goto thread_exit;
                }
                updateProfile(p, false);
            }
        }
    thread_exit:
        t->exit(0);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [this]() { //
        emit deviceWorkerFinished();
    });
    t->start(QThread::LowPriority);
}

static const quint32 FILE_BLOCK_MARKER[] = {
    0xbe250566, // 0
    0xba000000, // 1
    0xbb000001, // 2
    0xbb000002, // 3
    0xbb000003, // 4
    0xbb000004, // 5
    0xbe660525, // 6
};

void RTController::saveProfilesToFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::Truncate | QFile::WriteOnly)) {
        emit deviceError(EIO, tr("Unable to save file: %1").arg(fileName));
        return;
    }

    auto write = [](QFile *f, const void *p, const qsizetype size) -> int {
        if (f->write((char *) p, size) != size) {
            return EIO;
        }
        return 0;
    };

    quint32 length;
    foreach (const TProfile p, m_profiles) {
        if (p.name.length() == 0 || p.name.length() > HIDAPI_MAX_STR) {
            raiseError(EINVAL, "Invalid profile name.");
            goto error_exit;
        }
        if (p.index >= TYON_PROFILE_NUM) {
            raiseError(EINVAL, "Invalid profile index.");
            goto error_exit;
        }
        if (write(&f, &FILE_BLOCK_MARKER[0], sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, &p.index, sizeof(p.index))) {
            goto error_exit;
        }
        length = (quint32) p.name.length();
        if (write(&f, &length, sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, p.name.toLocal8Bit().constData(), length)) {
            goto error_exit;
        }
        if (write(&f, &FILE_BLOCK_MARKER[1], sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, &p.buttons.report_id, sizeof(p.buttons.report_id))) {
            goto error_exit;
        }
        if (write(&f, &p.buttons.size, sizeof(p.buttons.size))) {
            goto error_exit;
        }
        if (write(&f, &p.buttons.profile_index, sizeof(p.buttons.profile_index))) {
            goto error_exit;
        }
        length = TYON_PROFILE_BUTTON_NUM;
        if (write(&f, &length, sizeof(quint8))) {
            goto error_exit;
        }
        for (quint8 i = 0; i < TYON_PROFILE_BUTTON_NUM; i++) {
            if (write(&f, &p.buttons.buttons[i].type, sizeof(p.buttons.buttons[i].type))) {
                goto error_exit;
            }
            if (write(&f, &p.buttons.buttons[i].key, sizeof(p.buttons.buttons[i].key))) {
                goto error_exit;
            }
            if (write(&f, &p.buttons.buttons[i].modifier, sizeof(p.buttons.buttons[i].modifier))) {
                goto error_exit;
            }
        }
        if (write(&f, &FILE_BLOCK_MARKER[2], sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.report_id, sizeof(p.settings.report_id))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.size, sizeof(p.settings.size))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.profile_index, sizeof(p.settings.profile_index))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.advanced_sensitivity, sizeof(p.settings.advanced_sensitivity))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.sensitivity_x, sizeof(p.settings.sensitivity_x))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.sensitivity_y, sizeof(p.settings.sensitivity_y))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.cpi_levels_enabled, sizeof(p.settings.cpi_levels_enabled))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.cpi_active, sizeof(p.settings.cpi_active))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.talkfx_polling_rate, sizeof(p.settings.talkfx_polling_rate))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.lights_enabled, sizeof(p.settings.lights_enabled))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.color_flow, sizeof(p.settings.color_flow))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.light_effect, sizeof(p.settings.light_effect))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.effect_speed, sizeof(p.settings.effect_speed))) {
            goto error_exit;
        }
        if (write(&f, &FILE_BLOCK_MARKER[3], sizeof(quint32))) {
            goto error_exit;
        }
        length = TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM;
        if (write(&f, &length, sizeof(quint8))) {
            goto error_exit;
        }
        for (quint8 i = 0; i < TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM; i++) {
            if (write(&f, &p.settings.cpi_levels[i], sizeof(p.settings.cpi_levels[i]))) {
                goto error_exit;
            }
        }
        if (write(&f, &FILE_BLOCK_MARKER[4], sizeof(quint32))) {
            goto error_exit;
        }
        length = TYON_LIGHTS_NUM;
        if (write(&f, &length, sizeof(quint8))) {
            goto error_exit;
        }
        for (quint8 i = 0; i < TYON_LIGHTS_NUM; i++) {
            if (write(&f, &p.settings.lights[i].index, sizeof(p.settings.lights[i].index))) {
                goto error_exit;
            }
            if (write(&f, &p.settings.lights[i].red, sizeof(p.settings.lights[i].red))) {
                goto error_exit;
            }
            if (write(&f, &p.settings.lights[i].green, sizeof(p.settings.lights[i].green))) {
                goto error_exit;
            }
            if (write(&f, &p.settings.lights[i].blue, sizeof(p.settings.lights[i].blue))) {
                goto error_exit;
            }
            if (write(&f, &p.settings.lights[i].unused, sizeof(p.settings.lights[i].unused))) {
                goto error_exit;
            }
        }
        if (write(&f, &FILE_BLOCK_MARKER[5], sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, &p.settings.checksum, sizeof(p.settings.checksum))) {
            goto error_exit;
        }
        if (write(&f, &FILE_BLOCK_MARKER[6], sizeof(quint32))) {
            goto error_exit;
        }
    }

    // success
    goto func_exit;

error_exit:
    emit deviceError(EIO, tr("Unable to write file: %1").arg(fileName));

func_exit:
    f.flush();
    f.close();
    emit deviceWorkerFinished();
}

#define SafeDelete(p) \
    if (p) \
    delete p

void RTController::loadProfilesFromFile(const QString &fileName, bool raiseEvents)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        emit deviceError(EIO, tr("Unable to load file: %1").arg(fileName));
        return;
    }

    const QByteArray buffer = f.readAll();
    if (buffer.length() < (qsizetype) sizeof(TProfile)) {
        emit deviceError(EIO, tr("Invalid profiles file: %1").arg(fileName));
        f.close();
        return;
    }

    if (!raiseEvents) {
        blockSignals(true);
    }

    auto readNext = [](quint8 *p, void *value, qsizetype size) -> quint8 * {
        memcpy(value, p, size);
        p = (p + size);
        return p;
    };

    auto readMarker = [readNext](quint8 *p, quint32 value) -> quint8 * {
        quint32 mark = 0;
        p = readNext(p, &mark, sizeof(mark));
        return (mark == value ? p : nullptr);
    };

    TProfile *profile = 0;
    TProfiles profiles = {};
    quint8 *p = (quint8 *) buffer.constData();
    quint8 pfcount = 0;
    quint8 stage = 0;
    quint32 length;

    for (int i = 0; i < buffer.length() && pfcount < TYON_PROFILE_NUM; i++) {
        switch (stage) {
            case 0: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[0]))) {
                    emit deviceError(EIO, "Invalid data file. Header invalid");
                    goto func_exit;
                }
                profile = new TProfile();
                stage++;
                break;
            }
            case 1: {
                p = readNext(p, &profile->index, sizeof(profile->index));
                if (profile->index >= TYON_PROFILE_NUM) {
                    emit deviceError(EIO, "Invalid profile index.");
                    SafeDelete(profile);
                    goto func_exit;
                }
#ifdef QT_DEBUG
                qDebug("[HIDDEV] %s Read profile: %d", qPrintable(fileName), profile->index);
#endif
                p = readNext(p, &length, sizeof(quint32));
                if (length) {
                    quint8 c;
                    quint32 offset = 0;
                    for (quint32 i = 0; i < length && i < HIDAPI_MAX_STR; i++) {
                        p = readNext(p, &c, sizeof(quint8));
                        if (c >= 0x20 && c < 0x7f) { // only human readable
                            profile->name += QChar(c);
                            offset++;
                        }
                    }
                    if (offset != length) {
                        emit deviceError(EIO, "Invalid profile name.");
                        SafeDelete(profile);
                        goto func_exit;
                    }
                } else {
                    profile->name = tr("Profile %1").arg(profile->index);
                }
                stage++;
                break;
            }
            case 2: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[1]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileButtons *pb = &profile->buttons;
                p = readNext(p, &pb->report_id, sizeof(pb->report_id));
                p = readNext(p, &pb->size, sizeof(pb->size));
                p = readNext(p, &pb->profile_index, sizeof(pb->profile_index));
                if (pb->report_id != TYON_REPORT_ID_PROFILE_BUTTONS) {
                    emit deviceError(EIO, "Invalid button report identifier.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (pb->size != sizeof(TyonProfileButtons)) {
                    emit deviceError(EIO, "Invalid profile data.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (pb->profile_index >= TYON_PROFILE_NUM) {
                    emit deviceError(EIO, "Invalid button profile index.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_PROFILE_BUTTON_NUM) {
                    emit deviceError(EIO, "Invalid button count value.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                for (quint8 i = 0; i < TYON_PROFILE_BUTTON_NUM; i++) {
                    RoccatButton *b = &profile->buttons.buttons[i];
                    p = readNext(p, &b->type, sizeof(b->type));
                    p = readNext(p, &b->key, sizeof(b->key));
                    p = readNext(p, &b->modifier, sizeof(b->modifier));
                }
                stage++;
                break;
            }
            case 3: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[2]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &ps->report_id, sizeof(ps->report_id));
                p = readNext(p, &ps->size, sizeof(ps->size));
                p = readNext(p, &ps->profile_index, sizeof(ps->profile_index));
                if (ps->report_id != TYON_REPORT_ID_PROFILE_SETTINGS) {
                    emit deviceError(EIO, "Invalid button report identifier.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (ps->size != sizeof(TyonProfileSettings)) {
                    emit deviceError(EIO, "Invalid profile data.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (ps->profile_index >= TYON_PROFILE_NUM) {
                    emit deviceError(EIO, "Invalid button profile index.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                p = readNext(p, &ps->advanced_sensitivity, sizeof(ps->advanced_sensitivity));
                p = readNext(p, &ps->sensitivity_x, sizeof(ps->sensitivity_x));
                p = readNext(p, &ps->sensitivity_y, sizeof(ps->sensitivity_y));
                p = readNext(p, &ps->cpi_levels_enabled, sizeof(ps->cpi_levels_enabled));
                p = readNext(p, &ps->cpi_active, sizeof(ps->cpi_active));
                p = readNext(p, &ps->talkfx_polling_rate, sizeof(ps->talkfx_polling_rate));
                p = readNext(p, &ps->lights_enabled, sizeof(ps->lights_enabled));
                p = readNext(p, &ps->color_flow, sizeof(ps->color_flow));
                p = readNext(p, &ps->light_effect, sizeof(ps->light_effect));
                p = readNext(p, &ps->effect_speed, sizeof(ps->effect_speed));
                stage++;
                break;
            }
            case 4: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[3]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM) {
                    emit deviceError(EIO, "Invalid DPI level count value.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                for (quint8 i = 0; i < TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM; i++) {
                    p = readNext(p, &ps->cpi_levels[i], sizeof(ps->cpi_levels[i]));
                }
                stage++;
                break;
            }
            case 5: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[4]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_LIGHTS_NUM) {
                    emit deviceError(EIO, "Invalid light count value.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                for (quint8 i = 0; i < TYON_LIGHTS_NUM; i++) {
                    p = readNext(p, &ps->lights[i].index, sizeof(ps->lights[i].index));
                    p = readNext(p, &ps->lights[i].red, sizeof(ps->lights[i].red));
                    p = readNext(p, &ps->lights[i].green, sizeof(ps->lights[i].green));
                    p = readNext(p, &ps->lights[i].blue, sizeof(ps->lights[i].blue));
                    p = readNext(p, &ps->lights[i].unused, sizeof(ps->lights[i].unused));
                }
                stage++;
                break;
            }
            case 6: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[5]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &ps->checksum, sizeof(ps->checksum));
                stage++;
                break;
            }
            case 7: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[6]))) {
                    emit deviceError(EIO, "Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                updateProfile((*profile), true);
                SafeDelete(profile);
                profile = 0L;
                pfcount++;
                stage = 0; // next profile
                break;
            }
        }
    } // for

func_exit:
    f.close();
    emit deviceWorkerFinished();
    if (!raiseEvents) {
        blockSignals(false);
    }
}

void RTController::setupButton(const RoccatButton &rb, QPushButton *button)
{
    QKeySequence ks;

    // assign ROCCAT button type to push button
    button->setProperty("type", QVariant::fromValue(rb.type));
    button->setProperty("modifier", QVariant::fromValue(rb.modifier));
    button->setProperty("hiduid", QVariant::fromValue(rb.key));
    button->setText(buttonTypes().value(rb.type));

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

void RTController::assignButton(TyonButtonIndex type, TyonButtonType func, QKeyCombination kc)
{
    if ((qint8) type > TYON_PROFILE_BUTTON_NUM) {
        return;
    }
    if (!m_profiles.contains(activeProfileIndex())) {
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[HIDDEV] assignButton TYPE:" << type << "FUNC:" << func << "KC:" << kc;
#endif

    const TUidToQtKeyMap *keymap = nullptr;
    quint8 mods = 0;

    if (func == TYON_BUTTON_TYPE_SHORTCUT) {
        QLocale locale = QApplication::inputMethod()->locale();
        if (locale.language() == QLocale::German) {
            QString language = locale.languageToString(locale.language());
            qDebug() << "Aktuelle Tastatursprache:" << language;
            // y -> z & vs.
            if (kc.key() == Qt::Key_Y) {
                kc = QKeyCombination(kc.keyboardModifiers(), Qt::Key_Z);
            } else if (kc.key() == Qt::Key_Z) {
                kc = QKeyCombination(kc.keyboardModifiers(), Qt::Key_Y);
            }
        }

        // Translate QT key modifiers to ROCCAT Tyon modifiers
        auto toRoccatKMods = [](const Qt::KeyboardModifiers &km) -> quint8 {
            quint8 mods = 0;
            /* on Mac OSX Meta must be mapped to CTRL */
            if (km.testFlag(Qt::ShiftModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_SHIFT;
            if (km.testFlag(Qt::ControlModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_CTRL;
            if (km.testFlag(Qt::AltModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_ALT;
            if (km.testFlag(Qt::MetaModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_WIN;
            return mods;
        };

        const bool isKeyPad = kc.keyboardModifiers().testFlag(Qt::KeypadModifier);
        const Qt::KeyboardModifier testMod = (isKeyPad ? Qt::KeypadModifier : Qt::NoModifier);
        for (const TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
            if (kc.key() == p->qt_key && p->modifier == testMod) {
                keymap = p;
                break;
            }
        }
        if (!keymap) {
            return;
        }

        mods = toRoccatKMods(kc.keyboardModifiers());
    }

    TProfile p = m_profiles[activeProfileIndex()];
    RoccatButton *b = &p.buttons.buttons[type];
    b->type = func;
    b->modifier = (keymap != nullptr ? mods : 0);
    b->key = (keymap != nullptr ? keymap->uid_key : 0);
    updateProfile(p, true);
}

const QKeySequence RTController::toKeySequence(const RoccatButton &b) const
{
    // Translate ROCCAT Tyon key modifier to QT type
    auto toQtModifiers = [](quint8 modifier, const TUidToQtKeyMap *keymap) -> Qt::KeyboardModifiers {
        Qt::KeyboardModifiers km = {};
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_SHIFT) {
            km.setFlag(Qt::ShiftModifier, true);
        }
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_CTRL) {
            km.setFlag(Qt::ControlModifier, true);
        }
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_ALT) {
            km.setFlag(Qt::AltModifier, true);
        }
        if (modifier & ROCCAT_BUTTON_MODIFIER_BIT_WIN) {
            km.setFlag(Qt::MetaModifier, true);
        }
        /* Nummber keypad */
        if (keymap->modifier & Qt::KeypadModifier) {
            km.setFlag(Qt::KeypadModifier, true);
        }
        return km;
    };

    quint8 _rbk = b.key;
    QLocale locale = QApplication::inputMethod()->locale();
    QString language = locale.languageToString(locale.language());

    if (locale.language() == QLocale::German) {
        // y -> z & vs.
        if (_rbk == HID_UID_KB_Y) {
            _rbk = HID_UID_KB_Z;
        } else if (_rbk == HID_UID_KB_Z) {
            _rbk = HID_UID_KB_Y;
        }
    }

    const TUidToQtKeyMap *keymap = nullptr;
    for (const TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
        if (_rbk == p->uid_key) {
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

qint16 RTController::toSensitivityXValue(const TyonProfileSettings *settings) const
{
    return (settings->sensitivity_x - ROCCAT_SENSITIVITY_CENTER);
}

qint16 RTController::toSensitivityYValue(const TyonProfileSettings *settings) const
{
    return (settings->sensitivity_y - ROCCAT_SENSITIVITY_CENTER);
}

quint16 RTController::toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const
{
    if (index > TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM)
        return 0;
    return (settings->cpi_levels[index] >> 2) * 200;
}

void RTController::setActiveProfile(quint8 pix)
{
    if (m_activeProfile.profile_index != pix && pix < TYON_PROFILE_NUM) {
        m_activeProfile.profile_index = pix;
        emit profileIndexChanged(pix);
        emit profileChanged(m_profiles[pix]);
    }
}

void RTController::setProfileName(const QString &name, quint8 pix)
{
    if (m_profiles.contains(pix)) {
        TProfile p = m_profiles[pix];
        if (p.name != name) {
            if (p.name.isEmpty()) {
                raiseError(EINVAL, "Invalid profile name.");
                return;
            }
            p.name = name;
            updateProfile(p, true);
        }
    }
}

void RTController::setXSensitivity(qint16 sensitivity)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.sensitivity_x != sensitivity) {
            p.settings.sensitivity_x = sensitivity + ROCCAT_SENSITIVITY_CENTER;
            updateProfile(p, true);
        }
    }
}

void RTController::setYSensitivity(qint16 sensitivity)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.sensitivity_y != sensitivity) {
            p.settings.sensitivity_y = sensitivity + ROCCAT_SENSITIVITY_CENTER;
            updateProfile(p, true);
        }
    }
}

void RTController::setAdvancedSenitivity(bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (state) {
            p.settings.advanced_sensitivity |= ROCCAT_SENSITIVITY_ADVANCED_ON;
        } else {
            p.settings.advanced_sensitivity &= ~ROCCAT_SENSITIVITY_ADVANCED_ON;
        }
        updateProfile(p, true);
    }
}

void RTController::setDpiSlot(quint8 bit, bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (state) {
            p.settings.cpi_levels_enabled |= bit;
        } else {
            p.settings.cpi_levels_enabled &= ~bit;
        }
        updateProfile(p, true);
    }
}

void RTController::setActiveDpiSlot(quint8 id)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.cpi_active != id) {
            p.settings.cpi_active = id;
            updateProfile(p, true);
        }
    }
}

void RTController::setDpiLevel(quint8 index, quint16 value)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        quint8 level = ((value / 200) << 2);
        if (p.settings.cpi_levels[index] != level) {
            p.settings.cpi_levels[index] = level;
            updateProfile(p, true);
        }
    }
}

void RTController::setLightsEffect(quint8 value)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.light_effect != value) {
            p.settings.light_effect = value;
            updateProfile(p, true);
        }
    }
}

void RTController::setColorFlow(quint8 value)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.color_flow != value) {
            p.settings.color_flow = value;
            updateProfile(p, true);
        }
    }
}

static inline void _set_nibble8(quint8 *byte, uint nibble, quint8 value)
{
    if (nibble == 1)
        *byte = (*byte & 0x0f) | ((value << 4) & 0xf0);
    else
        *byte = (*byte & 0xf0) | (value & 0x0f);
}

static inline quint8 _get_nibble8(quint8 byte, uint nibble)
{
    if (nibble == 1)
        return (byte & 0xf0) >> 4;
    else
        return byte & 0x0f;
}

quint8 RTController::talkFxPollRate(const TyonProfileSettings *settings) const
{
    quint8 value = 0;
    if (settings) {
        value = settings->talkfx_polling_rate;
        value = _get_nibble8(value, ROCCAT_NIBBLE_LOW);
    }
    return value;
}

void RTController::setTalkFxPollRate(quint8 rate)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        quint8 value = p.settings.talkfx_polling_rate;
        _set_nibble8(&value, ROCCAT_NIBBLE_LOW, rate);
        if (p.settings.talkfx_polling_rate != value) {
            p.settings.talkfx_polling_rate = value;
            updateProfile(p, true);
        }
    }
}

bool RTController::talkFxState(const TyonProfileSettings *settings) const
{
    bool state = false;
    if (m_profiles.contains(activeProfileIndex())) {
        quint8 value = settings->talkfx_polling_rate;
        value = _get_nibble8(value, ROCCAT_NIBBLE_HIGH);
        state = (value == TYON_PROFILE_SETTINGS_TALKFX_ON);
    }
    return state;
}

void RTController::setTalkFxState(bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        quint8 value = p.settings.talkfx_polling_rate;
        if (state) {
            _set_nibble8(&value, ROCCAT_NIBBLE_HIGH, TYON_PROFILE_SETTINGS_TALKFX_ON);
        } else {
            _set_nibble8(&value, ROCCAT_NIBBLE_HIGH, TYON_PROFILE_SETTINGS_TALKFX_OFF);
        }
        if (p.settings.talkfx_polling_rate != value) {
            p.settings.talkfx_polling_rate = value;
            updateProfile(p, true);
        }
    }
}

void RTController::setDcuState(TyonControlUnitDcu state)
{
    m_controlUnit.dcu = state;
}

void RTController::setTcuState(bool state)
{
    if (state) {
        m_controlUnit.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    } else {
        m_controlUnit.tcu = TYON_TRACKING_CONTROL_UNIT_OFF;
    }
}

void RTController::setLightWheelEnabled(bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (state) {
            p.settings.lights_enabled |= TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_WHEEL;
        } else {
            p.settings.lights_enabled &= ~TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_WHEEL;
        }
        updateProfile(p, true);
    }
}

void RTController::setLightBottomEnabled(bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (state) {
            p.settings.lights_enabled |= TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_BOTTOM;
        } else {
            p.settings.lights_enabled &= ~TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_BOTTOM;
        }
        updateProfile(p, true);
    }
}

void RTController::setLightCustomColorEnabled(bool state)
{
    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (state) {
            p.settings.lights_enabled |= TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR;
        } else {
            p.settings.lights_enabled &= ~TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR;
        }
        updateProfile(p, true);
    }
}

TyonLight RTController::toDeviceColor(TyonLightType target, const QColor &color) const
{
    if ((quint8) target >= TYON_LIGHTS_NUM) {
        qWarning("[HIDDEV] toDeviceColor(): Invalid light index. 0 or 1 expected.");
        return {};
    }
    const TProfile p = m_profiles[activeProfileIndex()];
    const TyonLight tl = p.settings.lights[target];
    TyonLight info = {};
    info.index = tl.index;
    info.unused = tl.unused;
    info.red = color.red();
    info.green = color.green();
    info.blue = color.blue();
    return info;
}

QColor RTController::toScreenColor(const TyonLight &light, bool isCustom) const
{
    if (isCustom) {
        return QColor::fromRgb(light.red, light.green, light.blue);
    }
    if (light.index >= m_colors.count()) {
        qWarning("[HIDDEV] toScreenColor(): Invalid color index: %d", light.index);
        return Qt::red;
    }
    const TyonLight tci = m_colors.value(light.index).deviceColors;
    return QColor::fromRgb(tci.red, tci.green, tci.blue);
}

void RTController::setLightColor(TyonLightType target, const TyonLight &color)
{
    if ((quint8) target >= TYON_LIGHTS_NUM) {
        qWarning("[HIDDEV] Invalid light index. 0 or 1 expected.");
        return;
    }

    if (m_profiles.contains(activeProfileIndex())) {
        TProfile p = m_profiles[activeProfileIndex()];
        if (p.settings.lights[target].red != color.red        //
            || p.settings.lights[target].green != color.green //
            || p.settings.lights[target].blue != color.blue   //
            || p.settings.lights[target].index != color.index //
            || p.settings.lights[target].unused != color.unused) {
            p.settings.lights[target].index = color.index;
            p.settings.lights[target].red = color.red;
            p.settings.lights[target].green = color.green;
            p.settings.lights[target].blue = color.blue;
            p.settings.lights[target].unused = color.unused;
            updateProfile(p, true);
        }
    }
}

QString RTController::profileName() const
{
    if (m_profiles.contains(activeProfileIndex())) {
        return m_profiles[activeProfileIndex()].name;
    }
    return "";
}

TyonControlUnitDcu RTController::dcuState() const
{
    return static_cast<TyonControlUnitDcu>(m_controlUnit.dcu);
}

TyonControlUnitTcu RTController::tcuState() const
{
    return static_cast<TyonControlUnitTcu>(m_controlUnit.tcu);
}

uint RTController::tcuMedian() const
{
    return m_controlUnit.median;
}

quint8 RTController::minimumXCelerate() const
{
    return m_info.xcelerator_min;
}

quint8 RTController::maximumXCelerate() const
{
    return m_info.xcelerator_max;
}

quint8 RTController::middleXCelerate() const
{
    return m_info.xcelerator_mid;
}

void RTController::xcStartCalibration()
{
    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t]() { //
        xcCalibWriteStart();
        t->exit(0);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [this](QObject *) { //
        emit deviceWorkerFinished();
    });
    t->start(QThread::IdlePriority);
    QThread::yieldCurrentThread();
    QThread::msleep(500);
}

void RTController::xcStopCalibration()
{
    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t]() { //
        xcCalibWriteEnd();
        t->exit(0);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [this](QObject *) { //
        emit deviceWorkerFinished();
    });
    t->start(QThread::IdlePriority);
    QThread::yieldCurrentThread();
    QThread::msleep(200);
}

void RTController::tcuSensorTest(TyonControlUnitDcu dcu, uint median)
{
    tcuWriteTest(dcu, median);
}

void RTController::tcuSensorAccept(TyonControlUnitDcu dcuState, uint median)
{
    tcuWriteAccept(dcuState, median);
}

void RTController::tcuSensorCancel(TyonControlUnitDcu dcuState)
{
    tcuWriteCancel(dcuState);
}

void RTController::tcuSensorCaptureImage()
{
    tcuWriteSensorImageCapture();
}

void RTController::tcuSensorReadImage()
{
    tcuReadSensorImage();
}

int RTController::tcuSensorReadMedian(TyonSensorImage *image)
{
    return sensorMedianOfImage(image);
}

void RTController::xcApplyCalibration(quint8 min, quint8 mid, quint8 max)
{
    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, min, mid, max]() { //
        qInfo("[HIDDEV] Apply X-Celerator min=%d mid=%d max=%d", min, mid, max);
        xcCalibWriteData(min, mid, max);
        t->exit(0);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [this](QObject *) { //
        emit deviceWorkerFinished();
    });
    t->start(QThread::IdlePriority);
    QThread::yieldCurrentThread();
    QThread::msleep(500);
}

inline bool RTController::readProfiles(quint8 pix)
{
    /* select profile settings store */
    if (!selectProfileSettings(pix)) {
        return false;
    }

    /* read profile settings */
    if (!readProfileSettings()) {
        return false;
    }

    /* select profile buttons store */
    if (!selectProfileButtons(pix)) {
        return false;
    }

    /* read profile buttons */
    if (!readProfileButtons()) {
        return false;
    }

#if 0
    /* read all button slots including combined with EasyShift */
    for (quint8 bix = 0; bix < TYON_PROFILE_BUTTON_NUM; bix++) {
        if (!readButtonMacro(pix, bix)) {
            return false;
        }
    }
#endif

    // reset change flag
    setModified(pix, false);
    return true;
}

inline void RTController::raiseError(int error, const QString &message)
{
    qCritical("[HIDDEV] Error 0x%08x: %s", error, qPrintable(message));

    emit deviceError(error, message);
}

inline void RTController::internalSaveProfiles()
{
    QString fpath = QStandardPaths::writableLocation( //
        QStandardPaths::AppConfigLocation);

    QDir d(fpath);
    if (!d.exists()) {
        if (!d.mkpath(fpath)) {
            return;
        }
    }

    saveProfilesToFile(QDir::toNativeSeparators(fpath + "/profiles.rtpf"));
}

inline void RTController::setModified(quint8 pix, bool changed)
{
    if (m_profiles.contains(pix)) {
        TProfile p = m_profiles[pix];
        updateProfile(p, changed);
    }
}

inline void RTController::setModified(TProfile *p, bool changed)
{
    if (p->changed != changed) {
        p->changed = changed;
    }
}

inline void RTController::updateProfile(TProfile &p, bool changed)
{
    if (p.index >= TYON_PROFILE_NUM) {
        raiseError(EINVAL, "Invalid profile index.");
        return;
    }

    p.changed = changed;
    m_profiles[p.index] = p;

    // emit event only on active profile
    if (p.index == activeProfileIndex()) {
        emit profileChanged(p);
    }
}

inline bool RTController::readDeviceControl()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_CONTROL, sizeof(RoccatControl));
}

inline bool RTController::readActiveProfile()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_PROFILE, sizeof(TyonProfile));
}

inline bool RTController::readProfileSettings()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_PROFILE_SETTINGS, sizeof(TyonProfileSettings));
}

inline bool RTController::readProfileButtons()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_PROFILE_BUTTONS, sizeof(TyonProfileButtons));
}

inline bool RTController::readDeviceInfo()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_INFO, sizeof(TyonInfo));
}

inline bool RTController::talkRead()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_TALK, sizeof(TyonTalk));
}

inline bool RTController::readControlUnit()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_CONTROL_UNIT, sizeof(TyonControlUnit));
}

inline bool RTController::setDeviceState(bool state)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonDeviceState devstate;
    devstate.report_id = TYON_REPORT_ID_DEVICE_STATE;
    devstate.size = sizeof(TyonDeviceState);
    devstate.state = (state ? 0x01 : 0x00);

    const quint8 *buffer = (const quint8 *) &devstate;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, devstate.report_id, buffer, devstate.size);
}

inline bool RTController::tcuWriteTest(quint8 dcuState, uint median)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control.median = median;
    control.action = TYON_CONTROL_UNIT_ACTION_CANCEL;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::tcuWriteAccept(quint8 dcuState, uint median)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control.median = median;
    control.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::tcuWriteOff(quint8 dcuState)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_OFF;
    control.median = 0;
    control.action = TYON_CONTROL_UNIT_ACTION_OFF;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::tcuWriteTry(quint8 dcuState)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_UNDEFINED;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::tcuWriteCancel(quint8 dcuState)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_CANCEL;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::dcuWriteState(quint8 dcuState)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;

    const quint8 *buffer = (const quint8 *) &control;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, control.report_id, buffer, control.size);
}

inline bool RTController::tcuReadSensor()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_SENSOR, sizeof(TyonSensor));
}

inline bool RTController::tcuReadSensorImage()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->readHidMessage(hdt, TYON_REPORT_ID_SENSOR, sizeof(TyonSensorImage));
}

inline bool RTController::tcuReadSensorRegister(quint8 reg)
{
    if (!tcuWriteSensorCommand(TYON_SENSOR_ACTION_READ, reg, 0)) {
        return false;
    }
    return tcuReadSensor();
}

inline bool RTController::tcuWriteSensorCommand(quint8 action, quint8 reg, quint8 value)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonSensor sensor;
    sensor.report_id = TYON_REPORT_ID_SENSOR;
    sensor.action = action;
    sensor.reg = reg;
    sensor.value = value;

    const quint8 *buffer = (const quint8 *) &sensor;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, sensor.report_id, buffer, sizeof(TyonSensor));
}

inline bool RTController::tcuWriteSensorImageCapture()
{
    return tcuWriteSensorCommand(TYON_SENSOR_ACTION_FRAME_CAPTURE, 1, 0);
}

inline bool RTController::tcuWriteSensorRegister(quint8 reg, quint8 value)
{
    return tcuWriteSensorCommand(TYON_SENSOR_ACTION_WRITE, reg, value);
}

uint RTController::sensorMedianOfImage(TyonSensorImage const *image)
{
    uint i;
    ulong sum = 0;
    for (i = 0; i < TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE; ++i)
        sum += image->data[i];
    return (uint) (sum / (TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE));
}

inline bool RTController::xcCalibWriteStart()
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_START;

    const quint8 *buffer = (const quint8 *) &info;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, info.report_id, buffer, info.size);
}

inline bool RTController::xcCalibWriteEnd()
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_END;

    const quint8 *buffer = (const quint8 *) &info;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, info.report_id, buffer, info.size);
}

inline bool RTController::xcCalibWriteData(quint8 min, quint8 mid, quint8 max)
{
    if (!roccatControlCheck()) {
        return false;
    }

    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_DATA;
    info.xcelerator_min = min;
    info.xcelerator_mid = mid;
    info.xcelerator_max = max;

    const quint8 *buffer = (const quint8 *) &info;
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    return m_hid->writeHidMessage(hdt, info.report_id, buffer, info.size);
}

inline bool RTController::readButtonMacro(uint pix, uint bix)
{
    if (pix >= TYON_PROFILE_NUM) {
        raiseError(EINVAL, tr("Invalid profile index parameter."));
        return false;
    }

    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        raiseError(EINVAL, tr("Invalid button index parameter."));
        return false;
    }

    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    const TyonControlDataIndex dix1 = TYON_CONTROL_DATA_INDEX_MACRO_1;
    const TyonControlDataIndex dix2 = TYON_CONTROL_DATA_INDEX_MACRO_1;

    if (!selectMacro(pix, dix1, bix)) {
        return false;
    }

    if (!m_hid->readHidMessage(hdt, TYON_REPORT_ID_MACRO, sizeof(TyonMacro1))) {
        return false;
    }

    if (!selectMacro(pix, dix2, bix)) {
        return false;
    }

    if (!m_hid->readHidMessage(hdt, TYON_REPORT_ID_MACRO, sizeof(TyonMacro2))) {
        return false;
    }

    return true;
}

inline bool RTController::selectProfileSettings(uint pix)
{
    if (pix >= TYON_PROFILE_NUM) {
        raiseError(EINVAL, tr("Invalid profile index."));
        return false;
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_SETTINGS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    m_requestedProfile = pix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile settings. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return roccatControlWrite(_dix, _req);
}

inline bool RTController::selectProfileButtons(uint pix)
{
    if (pix >= TYON_PROFILE_NUM) {
        raiseError(EINVAL, tr("Invalid profile index."));
        return false;
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_BUTTONS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    m_requestedProfile = pix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile buttons. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return roccatControlWrite(_dix, _req);
}

inline bool RTController::selectMacro(uint pix, uint dix, uint bix)
{
    if (pix >= TYON_PROFILE_NUM) {
        raiseError(EINVAL, tr("Invalid profile index."));
        return false;
    }
    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        raiseError(EINVAL, tr("Invalid macro index."));
        return false;
    }

    const quint8 _dix = (dix | pix);
    const quint8 _req = bix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select macro. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return roccatControlWrite(_dix, _req);
}

inline bool RTController::talkWriteReport(TyonTalk *tyonTalk)
{
    if (!roccatControlCheck()) {
        return false;
    }

    tyonTalk->report_id = TYON_REPORT_ID_TALK;
    tyonTalk->size = sizeof(TyonTalk);

    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    const quint8 *buffer = (const quint8 *) tyonTalk;

    return m_hid->writeHidMessage(hdt, tyonTalk->report_id, buffer, tyonTalk->size);
}

inline bool RTController::talkWriteKey(quint8 easyshift, quint8 easyshift_lock, quint8 easyaim)
{
    TyonTalk tyonTalk = {};
    tyonTalk.easyaim = easyaim;
    tyonTalk.easyshift = easyshift;
    tyonTalk.easyshift_lock = easyshift_lock;
    tyonTalk.fx_status = TYON_TALKFX_STATE_UNUSED;
    return talkWriteReport(&tyonTalk);
}

inline bool RTController::talkWriteEasyshift(quint8 state)
{
    return talkWriteKey(state, TYON_TALK_EASYSHIFT_UNUSED, TYON_TALK_EASYAIM_UNUSED);
}

inline bool RTController::talkWriteEasyshiftLock(quint8 state)
{
    return talkWriteKey(TYON_TALK_EASYSHIFT_UNUSED, state, TYON_TALK_EASYAIM_UNUSED);
}

inline bool RTController::talkWriteEasyAim(quint8 state)
{
    return talkWriteKey(TYON_TALK_EASYSHIFT_UNUSED, TYON_TALK_EASYSHIFT_UNUSED, state);
}

inline bool RTController::talkWriteFxData(TyonTalk *tyonTalk)
{
    tyonTalk->easyshift = TYON_TALK_EASYSHIFT_UNUSED;
    tyonTalk->easyshift_lock = TYON_TALK_EASYSHIFT_UNUSED;
    tyonTalk->easyaim = TYON_TALK_EASYAIM_UNUSED;
    return talkWriteReport(tyonTalk);
}

inline bool RTController::talkWriteFx(quint32 effect, quint32 ambient_color, quint32 event_color)
{
    uint zone;
    TyonTalk tyonTalk = {};
    zone = (effect & ROCCAT_TALKFX_ZONE_BIT_MASK) >> ROCCAT_TALKFX_ZONE_BIT_SHIFT;
    tyonTalk.zone = (zone == ROCCAT_TALKFX_ZONE_AMBIENT) ? TYON_TALKFX_ZONE_AMBIENT : TYON_TALKFX_ZONE_EVENT;
    tyonTalk.effect = (effect & ROCCAT_TALKFX_EFFECT_BIT_MASK) >> ROCCAT_TALKFX_EFFECT_BIT_SHIFT;
    tyonTalk.speed = (effect & ROCCAT_TALKFX_SPEED_BIT_MASK) >> ROCCAT_TALKFX_SPEED_BIT_SHIFT;
    tyonTalk.ambient_red = (ambient_color & ROCCAT_TALKFX_COLOR_RED_MASK) >> ROCCAT_TALKFX_COLOR_RED_SHIFT;
    tyonTalk.ambient_green = (ambient_color & ROCCAT_TALKFX_COLOR_GREEN_MASK) >> ROCCAT_TALKFX_COLOR_GREEN_SHIFT;
    tyonTalk.ambient_blue = (ambient_color & ROCCAT_TALKFX_COLOR_BLUE_MASK) >> ROCCAT_TALKFX_COLOR_BLUE_SHIFT;
    tyonTalk.event_red = (event_color & ROCCAT_TALKFX_COLOR_RED_MASK) >> ROCCAT_TALKFX_COLOR_RED_SHIFT;
    tyonTalk.event_green = (event_color & ROCCAT_TALKFX_COLOR_GREEN_MASK) >> ROCCAT_TALKFX_COLOR_GREEN_SHIFT;
    tyonTalk.event_blue = (event_color & ROCCAT_TALKFX_COLOR_BLUE_MASK) >> ROCCAT_TALKFX_COLOR_BLUE_SHIFT;
    tyonTalk.fx_status = ROCCAT_TALKFX_STATE_ON;
    return talkWriteFxData(&tyonTalk);
}

inline bool RTController::talkWriteFxState(quint8 state)
{
    TyonTalk tyonTalk = {};
    tyonTalk.fx_status = state;
    return talkWriteFxData(&tyonTalk);
}

inline bool RTController::roccatControlCheck()
{
    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    const quint32 rid = TYON_REPORT_ID_CONTROL;

    bool ok = true;

    while (ok) {
        RoccatControl ctl = {};
        ctl.report_id = TYON_REPORT_ID_CONTROL;
        qsizetype length = sizeof(RoccatControl);
        quint8 *buffer = (quint8 *) &ctl;
        if (!(ok = m_hid->readHidMessage(hdt, rid, buffer, length))) {
            break;
        }
        switch (ctl.value) {
            case ROCCAT_CONTROL_VALUE_STATUS_OK: {
                goto func_exit;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_BUSY: {
                QThread::msleep(500);
                break;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_1:
            case ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_2: {
                raiseError(ctl.value, tr("Got critical device status"));
                ok = false;
                goto func_exit;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_INVALID: {
                raiseError(ctl.value, tr("Got invalid device status"));
                ok = false;
                goto func_exit;
            }
            default: {
                raiseError(ctl.value, tr("Got unknown device error"));
                ok = false;
                goto func_exit;
            }
        }
    }

func_exit:
    return ok;
}

inline bool RTController::roccatControlWrite(uint pix, uint req)
{
    if (!roccatControlCheck()) {
        return false;
    }

    RoccatControl control = {};
    control.report_id = TYON_REPORT_ID_CONTROL;
    control.value = pix;
    control.request = req;

    const THidDeviceType hdt = THidDeviceType::HidMouseControl;
    const quint8 *buffer = (const quint8 *) &control;

    return m_hid->writeHidMessage(hdt, control.report_id, buffer, sizeof(RoccatControl));
}
