#include "rthiddevice.h"
#include "hid_uid.h"
#include "rttypes.h"
#include <IOKit/hid/IOHIDManager.h>
#include <hidapi.h>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QtGui/qkeysequence.h>

//#undef QT_DEBUG

#ifdef QT_DEBUG
#include "rthiddevicedbg.hpp"
#endif

Q_DECLARE_OPAQUE_POINTER(IOHIDDeviceRef)

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

// -------------------------------------------------------------
//
#ifdef QT_DEBUG
static inline void debugSettings(const RTHidDevice::TProfile profile, quint8 currentPix);
static inline void debugButtons(const RTHidDevice::TProfile profile, quint8 currentPix);
static inline void debugDevInfo(TyonInfo *p);
static inline void debugDevice(IOHIDDeviceRef device);
#endif

static inline void _deviceAttachedCallback(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
#ifdef QT_DEBUG
    debugDevice(device);
#endif

    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    ctx->onDeviceFound(device);
}

static inline void _deviceRemovedCallback(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    ctx->onDeviceRemoved(device);
}

// Callback for IOHIDDeviceSetReportWithCallback
static inline void _reportCallback( //
    void *context,
    IOReturn result,
    void *,
    IOHIDReportType type,
    uint32_t reportID,
    uint8_t *report,
    CFIndex reportLength)
{
#if 0
    qDebug("[HIDDEV] (WRCB) IOHIDDeviceSetReportWithCallback result=%d", result);
    qDebug("[HIDDEV] (WRCB) RT=%d RID=%d SIZE=%ld CTX=%p", type, reportID, reportLength, context);
#else
    Q_UNUSED(type)
#endif

    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    const QByteArray data((char *) report, reportLength);
    ctx->onSetReportCallback(result, reportID, data.length(), data);
}

static inline IOHIDReportType toMacOSReportType(uint ep)
{
    IOHIDReportType hrt;
    switch (ep) {
        case TYON_INTERFACE_MOUSE: {
            hrt = kIOHIDReportTypeFeature;
            break;
        }
        case TYON_INTERFACE_KEYBOARD: {
            hrt = kIOHIDReportTypeInput;
            break;
        }
        case TYON_INTERFACE_JOYSTICK: {
            hrt = kIOHIDReportTypeInput;
            break;
        }
        case TYON_INTERFACE_MISC: {
            hrt = kIOHIDReportTypeOutput;
            break;
        }
        default: {
            hrt = kIOHIDReportTypeFeature;
        }
    }
    return hrt;
}

// -------------------------------------------------------------
//

RTHidDevice::RTHidDevice(QObject *parent)
    : QObject{parent}
    , m_manager(nullptr)
    , m_devices()
    , m_info()
    , m_profile()
    , m_profiles()
    , m_mutex()
    , m_isCBComplete(false)
    , m_hasHidApi((hid_init() == 0))
{
    qRegisterMetaType<IOHIDDeviceRef>();
    initializeProfiles();
}

RTHidDevice::~RTHidDevice()
{
    releaseManager();
    hid_exit();
}

void RTHidDevice::lookupDevice()
{
    m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!m_manager) {
        qCritical("[HIDDEV] Failed to create HID manager.");
        return;
    }

    // Set up matching dictionary for Roccat Tyon devices
    CFMutableArrayRef filter = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

    // Constants for Roccat Tyon mouse Black and White variants
    const uint32_t vendorId = USB_DEVICE_ID_VENDOR_ROCCAT;
    const uint32_t productIds[2] =        //
        {USB_DEVICE_ID_ROCCAT_TYON_BLACK, //
         USB_DEVICE_ID_ROCCAT_TYON_WHITE};

    auto createMatching = [](uint32_t vendorId, uint32_t productId) -> CFMutableDictionaryRef {
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable( //
            kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        if (!dict)
            return nullptr;

        CFNumberRef vendorIdRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendorId);
        CFNumberRef productIdRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &productId);

        if (vendorIdRef) {
            CFDictionarySetValue(dict, CFSTR(kIOHIDVendorIDKey), vendorIdRef);
            CFRelease(vendorIdRef);
        }

        if (productIdRef) {
            CFDictionarySetValue(dict, CFSTR(kIOHIDProductIDKey), productIdRef);
            CFRelease(productIdRef);
        }

        return dict;
    };

    CFMutableDictionaryRef dict;
    for (auto productId : productIds) {
        if ((dict = createMatching(vendorId, productId))) {
            CFArrayAppendValue(filter, dict);
            CFRelease(dict);
        }
    }

    IOHIDManagerSetDeviceMatchingMultiple(m_manager, filter);
    CFRelease(filter);

    // Register for device attached and removed callbacks and schedule in runloop
    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, _deviceAttachedCallback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, _deviceRemovedCallback, this);
    IOHIDManagerScheduleWithRunLoop(m_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // Open HID manager
    IOReturn result = IOHIDManagerOpen(m_manager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess && result != -536870174) {
        qCritical("Failed to open HID manager: %d", result);
        CFRelease(m_manager);
        m_manager = nullptr;
        return;
    }

    emit lookupStarted();
}

bool RTHidDevice::resetProfiles()
{
    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_RESET;
    quint8 *buffer = (quint8 *) &info;

    m_profiles.clear();
    m_profile = {};
    m_info = {};

    initializeProfiles();

    IOReturn ret = kIOReturnSuccess;
    foreach (IOHIDDeviceRef device, m_devices) {
        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            goto func_exit;
        }
        if ((ret = hidSetReportRaw(device, buffer, info.size)) != kIOReturnSuccess) {
            goto func_exit;
        }
    }

func_exit:
    QThread::msleep(1000);
    releaseManager();
    lookupDevice();

    return ret == kIOReturnSuccess;
}

bool RTHidDevice::saveProfilesToDevice()
{
    emit saveProfilesStarted();

    auto writeIndex = [this](IOHIDDeviceRef device) -> IOReturn {
        IOReturn ret = kIOReturnSuccess;
        CFIndex length = sizeof(TyonProfile);
        quint8 *buffer = (quint8 *) &m_profile;

        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            return ret;
        }
        if ((ret = hidSetReportRaw(device, buffer, length)) != kIOReturnSuccess) {
            return ret;
        }

        QThread::yieldCurrentThread();

        return ret;
    };
    auto writeProfile = [this](IOHIDDeviceRef device, const TProfile &p) -> IOReturn {
        IOReturn ret = kIOReturnSuccess;
        CFIndex length;
        quint8 *buffer;

        length = sizeof(TyonProfileSettings);
        buffer = (quint8 *) &p.settings;

        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            return ret;
        }
        if ((ret = hidSetReportRaw(device, buffer, length)) != kIOReturnSuccess) {
            return ret;
        }

        length = sizeof(TyonProfileButtons);
        buffer = (quint8 *) &p.buttons;

        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            return ret;
        }
        if ((ret = hidSetReportRaw(device, buffer, length)) != kIOReturnSuccess) {
            return ret;
        }

        QThread::yieldCurrentThread();

        return ret;
    };

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, writeIndex, writeProfile]() { //
        IOReturn ret;
        foreach (IOHIDDeviceRef device, m_devices) {
            if ((ret = writeIndex(device)) != kIOReturnSuccess) {
                goto thread_exit;
            }
            foreach (const TProfile p, m_profiles) {
                if (p.changed) {
                    if ((ret = writeProfile(device, p)) != kIOReturnSuccess) {
                        goto thread_exit;
                    }
                }
            }
        }
    thread_exit:
        t->exit(ret);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(
        t,
        &QThread::destroyed,
        this,
        [this]() {
            emit saveProfilesFinished();
            releaseManager();
            lookupDevice();
        },
        Qt::QueuedConnection);
    t->start(QThread::LowPriority);

    return true;
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

bool RTHidDevice::saveProfilesToFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::Truncate | QFile::WriteOnly)) {
        return false;
    }
    quint32 length;

    auto write = [](QFile *f, const void *p, const qsizetype size) -> IOReturn {
        if (f->write((char *) p, size) != size) {
            return kIOReturnIOError;
        }
        return kIOReturnSuccess;
    };

    foreach (const TProfile p, m_profiles) {
        if (write(&f, &FILE_BLOCK_MARKER[0], sizeof(quint32))) {
            goto error_exit;
        }
        if (write(&f, &p.index, sizeof(p.index))) {
            goto error_exit;
        }
        length = p.name.length();
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
    goto func_exit;

error_exit:
    qCritical("[HIDEV] Unable to write file: %s", qPrintable(fileName));

func_exit:
    f.flush();
    f.close();
    return true;
}

#define SafeDelete(p) \
    if (p) \
    delete p

bool RTHidDevice::loadProfilesFromFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        return false;
    }

    qApp->processEvents();
    QThread::yieldCurrentThread();

    const QByteArray buffer = f.readAll();
    if (buffer.length() < (qsizetype) sizeof(TProfile)) {
        f.close();
        return false;
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
                    qCritical("[HIDDEV] Invalid data file. Header invalid");
                    goto func_exit;
                }
                profile = new TProfile();
                stage++;
                break;
            }
            case 1: {
                p = readNext(p, &profile->index, sizeof(profile->index));
                if (profile->index >= TYON_PROFILE_NUM) {
                    qCritical("[HIDDEV] Invalid profile index.");
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
                        qCritical("[HIDDEV] Invalid profile name.");
                        SafeDelete(profile);
                        goto func_exit;
                    }
                }
                stage++;
                break;
            }
            case 2: {
                if (!(p = readMarker(p, FILE_BLOCK_MARKER[1]))) {
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileButtons *pb = &profile->buttons;
                p = readNext(p, &pb->report_id, sizeof(pb->report_id));
                p = readNext(p, &pb->size, sizeof(pb->size));
                p = readNext(p, &pb->profile_index, sizeof(pb->profile_index));
                if (pb->report_id != TYON_REPORT_ID_PROFILE_BUTTONS) {
                    qCritical("[HIDDEV] Invalid button report identifier.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (pb->size != sizeof(TyonProfileButtons)) {
                    qCritical("[HIDDEV] Invalid profile data.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (pb->profile_index >= TYON_PROFILE_NUM) {
                    qCritical("[HIDDEV] Invalid button profile index.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_PROFILE_BUTTON_NUM) {
                    qCritical("[HIDDEV] Invalid button count value.");
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
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &ps->report_id, sizeof(ps->report_id));
                p = readNext(p, &ps->size, sizeof(ps->size));
                p = readNext(p, &ps->profile_index, sizeof(ps->profile_index));
                if (ps->report_id != TYON_REPORT_ID_PROFILE_SETTINGS) {
                    qCritical("[HIDDEV] Invalid button report identifier.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (ps->size != sizeof(TyonProfileSettings)) {
                    qCritical("[HIDDEV] Invalid profile data.");
                    SafeDelete(profile);
                    goto func_exit;
                }
                if (ps->profile_index >= TYON_PROFILE_NUM) {
                    qCritical("[HIDDEV] Invalid button profile index.");
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
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM) {
                    qCritical("[HIDDEV] Invalid DPI level count value.");
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
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                TyonProfileSettings *ps = &profile->settings;
                p = readNext(p, &length, sizeof(quint8));
                if (length != TYON_LIGHTS_NUM) {
                    qCritical("[HIDDEV] Invalid light count value.");
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
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
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
                    qCritical("[HIDDEV] Invalid data file. Stage marker invalid");
                    SafeDelete(profile);
                    goto func_exit;
                }
                profile->changed = true;
                profiles[profile->index] = (*profile);
                SafeDelete(profile);
                profile = 0L;
                pfcount++;
                stage = 0; // next profile
                break;
            }
        }
    } // for

    // success, update UI
    m_profiles.clear();
    foreach (const TProfile p, profiles) {
        m_profiles[p.index] = p;
        emit profileChanged(p);
    }

func_exit:
    f.close();
    return true;
}

void RTHidDevice::assignButton(TyonButtonIndex type, TyonButtonType func, const QKeyCombination &kc)
{
    if ((qint8) type > TYON_PROFILE_BUTTON_NUM) {
        return;
    }
    if (!m_profiles.contains(profileIndex())) {
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "[HIDDEV] assignButton TYPE:" << type << "FUNC:" << func << "KC:" << kc;
#endif

    TUidToQtKeyMap *keymap = nullptr;
    quint8 mods = 0;

    if (func == TYON_BUTTON_TYPE_SHORTCUT) {
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
            if (km.testFlag(Qt::MetaModifier))
                mods |= ROCCAT_BUTTON_MODIFIER_BIT_WIN;
            return mods;
        };

        bool isKeyPad = kc.keyboardModifiers().testFlag(Qt::KeypadModifier);
        Qt::KeyboardModifier testMod = (isKeyPad ? Qt::KeypadModifier : Qt::NoModifier);
        for (TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
            if (kc.key() == p->qt_key && p->modifier == testMod) {
                keymap = p;
                break;
            }
        }
        if (!keymap) {
            return;
        }

        mods = toQtMods2Roccat(kc.keyboardModifiers());
    }

    TProfile p = m_profiles[profileIndex()];
    RoccatButton *b = &p.buttons.buttons[type];
    b->type = func;
    b->modifier = (keymap != nullptr ? mods : 0);
    b->key = (keymap != nullptr ? keymap->uid_key : 0);
    p.changed = true;
    m_profiles[profileIndex()] = p;
    emit profileChanged(p);
}

const QKeySequence RTHidDevice::toKeySequence(const RoccatButton &b) const
{
    // Translate ROCCAT Tyon key modifier to QT type
    auto toQtModifiers = [](quint8 modifier, TUidToQtKeyMap *keymap) -> Qt::KeyboardModifiers {
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

qint16 RTHidDevice::toSensitivityXValue(const TyonProfileSettings *settings) const
{
    return (settings->sensitivity_x - ROCCAT_SENSITIVITY_CENTER);
}

qint16 RTHidDevice::toSensitivityYValue(const TyonProfileSettings *settings) const
{
    return (settings->sensitivity_y - ROCCAT_SENSITIVITY_CENTER);
}

quint16 RTHidDevice::toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const
{
    if (index > TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM)
        return 0;
    return (settings->cpi_levels[index] >> 2) * 200;
}

void RTHidDevice::setActiveProfile(quint8 pix)
{
    if (m_profile.profile_index != pix && pix < TYON_PROFILE_NUM) {
        m_profile.profile_index = pix;
        emit profileIndexChanged(pix);
        emit profileChanged(m_profiles[pix]);
    }
}

void RTHidDevice::setProfileName(const QString &name, quint8 pix)
{
    if (m_profiles.contains(pix)) {
        TProfile p = m_profiles[pix];
        if (p.name != name) {
            p.name = name;
            p.changed = true;
            m_profiles[pix] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setXSensitivity(quint8 sensitivity)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.sensitivity_x != sensitivity) {
            p.settings.sensitivity_x = sensitivity;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setYSensitivity(quint8 sensitivity)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.sensitivity_y != sensitivity) {
            p.settings.sensitivity_y = sensitivity;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setAdvancedSenitivity(quint8 bit, bool state)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (state) {
            p.settings.advanced_sensitivity |= bit;
        } else {
            p.settings.advanced_sensitivity &= ~bit;
        }
        p.changed = true;
        m_profiles[profileIndex()] = p;
        emit profileChanged(p);
    }
}

void RTHidDevice::setPollRate(quint8 rate)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.talkfx_polling_rate != rate) {
            p.settings.talkfx_polling_rate = rate;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setDpiSlot(quint8 bit, bool state)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (state) {
            p.settings.cpi_levels_enabled |= bit;
        } else {
            p.settings.cpi_levels_enabled &= ~bit;
        }
        p.changed = true;
        m_profiles[profileIndex()] = p;
        emit profileChanged(p);
    }
}

void RTHidDevice::setActiveDpiSlot(quint8 id)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.cpi_active != id) {
            p.settings.cpi_active = id;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setDpiLevel(quint8 index, quint16 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        quint8 level = ((value / 200) << 2);
        if (p.settings.cpi_levels[index] != level) {
            p.settings.cpi_levels[index] = level;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setLightsEnabled(quint8 bit, bool state)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (state) {
            p.settings.lights_enabled |= bit;
        } else {
            p.settings.lights_enabled &= ~bit;
        }
        p.changed = true;
        m_profiles[profileIndex()] = p;
        emit profileChanged(p);
    }
}

void RTHidDevice::setLightsEffect(quint8 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.light_effect != value) {
            p.settings.light_effect = value;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setColorFlow(quint8 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.color_flow != value) {
            p.settings.color_flow = value;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setLightColorWheel(const TyonRmpLightInfo &color)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.lights[0].red != color.red        //
            || p.settings.lights[0].green != color.green //
            || p.settings.lights[0].blue != color.blue   //
            || p.settings.lights[0].index != color.index) {
            p.settings.lights[0].index = color.index;
            p.settings.lights[0].red = color.red;
            p.settings.lights[0].green = color.green;
            p.settings.lights[0].blue = color.blue;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setLightColorBottom(const TyonRmpLightInfo &color)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.lights[1].red != color.red        //
            || p.settings.lights[1].green != color.green //
            || p.settings.lights[1].blue != color.blue   //
            || p.settings.lights[1].index != color.index) {
            p.settings.lights[1].index = color.index;
            p.settings.lights[1].red = color.red;
            p.settings.lights[1].green = color.green;
            p.settings.lights[1].blue = color.blue;
            p.changed = true;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

QString RTHidDevice::profileName() const
{
    if (m_profiles.contains(profileIndex())) {
        return m_profiles[profileIndex()].name;
    }
    return "";
}

void RTHidDevice::onDeviceFound(IOHIDDeviceRef device)
{
    IOReturn ret;

    // some devices cannot open ?? 4 times called for product
    ret = IOHIDDeviceOpen(device, kIOHIDOptionsTypeSeizeDevice);
    if (ret != kIOReturnSuccess) {
        qDebug("[HIDDEV] Unable to open device: %p", device);
        return;
    }

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Device(%p) opened", device);
#endif

    /* skip if already read */
    if (m_devices.contains(device)) {
        return;
    }

    if ((ret = setDeviceState(true, device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to set device state.");
        goto error_exit;
    }

    /* read firmware release */
    if ((ret = readDeviceInfo(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read device info.");
        goto error_exit;
    }

    /* read current profile number */
    if ((ret = readActiveProfile(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read device profile.");
        goto error_exit;
    }

    /* read profile slots */
    for (quint8 pix = 0; pix < TYON_PROFILE_NUM; pix++) {
        if ((ret = readProfile(device, pix)) != kIOReturnSuccess) {
            qCritical("[HIDDEV] Unable to read device profile #%d", pix);
            goto error_exit;
        }
        QThread::yieldCurrentThread();
    }

    m_devices.append(device);

    if (m_profiles.count() >= TYON_PROFILE_NUM) {
        emit deviceFound();
    }
    return;

error_exit:
    IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
    emit deviceError(ENODEV, "Unable to load device data.");
}

void RTHidDevice::onDeviceRemoved(IOHIDDeviceRef device)
{
    if (m_devices.contains(device)) {
        m_devices.removeOne(device);
    }
    emit deviceRemoved();
}

void RTHidDevice::onSetReportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data)
{
    QMutexLocker lock(&m_mutex);

#ifdef QT_DEBUG
    if (length > 0) {
        qDebug("[HIDDEV] status=%d RID=0x%02x SIZE=%ld", status, rid, length);
        qDebug("[HIDDEV] data=%s", qPrintable(data.toHex(' ')));
    }
#else
    Q_UNUSED(status)
    Q_UNUSED(rid)
    Q_UNUSED(index)
    Q_UNUSED(length)
    Q_UNUSED(data)
#endif

    m_isCBComplete = true;
}

inline int RTHidDevice::readProfile(IOHIDDeviceRef device, quint8 pix)
{
    IOReturn ret;

    /* select profile settings store */
    if ((ret = selectProfileSettings(device, pix)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=0x%08x", pix, ret);
        return ret;
    }

    /* read profile settings */
    if ((ret = readProfileSettings(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read profile settings. ret=0x%08x", ret);
        return ret;
    }

    /* select profile buttons store */
    if ((ret = selectProfileButtons(device, pix)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=0x%08x", pix, ret);
        return ret;
    }

    /* read profile buttons */
    if ((ret = readProfileButtons(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read profile buttons. ret=0x%08x", ret);
        return ret;
    }

#if 0
    /* read all button slots including combined with EasyShift */
    for (quint8 bix = 0; bix < TYON_PROFILE_BUTTON_NUM; bix++) {
        if ((ret = readButtonMacro(device, pix, bix)) != kIOReturnSuccess) {
            if (ret != ENODATA) { // optional macros
                qCritical("[HIDDEV] Unable to read profile macro #%d. ret=0x%08x", bix, ret);
                return ret;
            }
        }
    }
#endif

    // reset change flag
    TProfile p = m_profiles[pix];
    p.changed = false;
    m_profiles[pix] = p;

    return kIOReturnSuccess;
}

inline int RTHidDevice::setDeviceState(bool state, IOHIDDeviceRef device)
{
    IOReturn ret;
    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonDeviceState device_state;
    device_state.report_id = TYON_REPORT_ID_DEVICE_STATE;
    device_state.size = sizeof(TyonDeviceState);
    device_state.state = (state ? 0x01 : 0x00);

    const quint8 *buf = (const quint8 *) &device_state;
    return hidSetReportRaw(device, buf, device_state.size);
}

inline void RTHidDevice::releaseDevices()
{
    IOHIDDeviceRef dev;
    while (!m_devices.isEmpty()) {
        if ((dev = m_devices.takeFirst())) {
            IOHIDDeviceClose(dev, kIOHIDOptionsTypeSeizeDevice);
        }
    }
    m_devices.clear();
}

inline void RTHidDevice::releaseManager()
{
    releaseDevices();

    if (m_manager) {
        IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
        CFRelease(m_manager);
    }
}

inline void RTHidDevice::initializeProfiles()
{
    for (quint8 i = 0; i < TYON_PROFILE_NUM; i++) {
        TProfile profile = {};
        profile.index = i;
        profile.name = tr("Default_%1").arg(profile.index);
        profile.changed = false;
        m_profiles[i] = profile;
    }
}

inline int RTHidDevice::readDeviceSpecial(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_SPECIAL, 256);
}

inline int RTHidDevice::readDeviceControl(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_CONTROL, 3);
}

inline int RTHidDevice::readActiveProfile(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_PROFILE, sizeof(TyonProfile));
}

inline int RTHidDevice::readProfileSettings(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_PROFILE_SETTINGS, sizeof(TyonProfileSettings));
}

inline int RTHidDevice::readProfileButtons(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_PROFILE_BUTTONS, sizeof(TyonProfileButtons));
}

inline int RTHidDevice::readDeviceInfo(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_INFO, sizeof(TyonInfo));
}

inline int RTHidDevice::readDevice0A(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_A, 8);
}

inline int RTHidDevice::readDeviceSensor(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_SENSOR, 4);
}

inline int RTHidDevice::readDeviceControlUnit(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_CONTROL_UNIT, 6);
}

inline int RTHidDevice::readDeviceTalk(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_TALK, 16);
}

inline int RTHidDevice::readDevice11(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_11, 16);
}

inline int RTHidDevice::readDevice1A(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_1A, 1029);
}

inline int RTHidDevice::readButtonMacro(IOHIDDeviceRef device, uint pix, uint bix)
{
    IOReturn ret;

    if (pix >= TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index parameter.");
        return EINVAL;
    }

    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        qCritical("[HIDDEV] Invalid button index parameter.");
        return EINVAL;
    }

    const TyonControlDataIndex dix1 = TYON_CONTROL_DATA_INDEX_MACRO_1;
    const TyonControlDataIndex dix2 = TYON_CONTROL_DATA_INDEX_MACRO_1;

    if ((ret = selectMacro(device, pix, dix1, bix))) {
        return ret;
    }

    if ((ret = hidGetReportById(device, TYON_REPORT_ID_MACRO, sizeof(TyonMacro1))) != kIOReturnSuccess) {
        return ret;
    }

    if ((ret = selectMacro(device, pix, dix2, bix))) {
        return ret;
    }

    if ((ret = hidGetReportById(device, TYON_REPORT_ID_MACRO, sizeof(TyonMacro2))) != kIOReturnSuccess) {
        return ret;
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::selectProfileSettings(IOHIDDeviceRef device, uint pix)
{
    if (pix >= TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_SETTINGS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile settings. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

inline int RTHidDevice::selectProfileButtons(IOHIDDeviceRef device, uint pix)
{
    if (pix >= TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_BUTTONS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile buttons. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

inline int RTHidDevice::selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix)
{
    if (pix >= TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }
    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        qCritical("[HIDDEV] Invalid macro index: %d", pix);
        return EINVAL;
    }

    const quint8 _dix = (dix | pix);
    const quint8 _req = bix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select macro. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

// Read ROCCAT Mouse report descriptor
inline int RTHidDevice::parsePayload(int rid, const quint8 *buffer, CFIndex length)
{
    QByteArray payload;

    // set descriptor length
    for (CFIndex i = 0; i < length; i++) {
        payload.append(buffer[i]);
    }

    TProfile profile = {};
    switch (rid) {
        case TYON_REPORT_ID_INFO: {
            TyonInfo *p = (TyonInfo *) payload.constData();
            memcpy(&m_info, p, sizeof(TyonInfo));
#ifdef QT_DEBUG
            debugDevInfo(&m_info);
#endif
            emit deviceInfo(m_info);
            break;
        }
        case TYON_REPORT_ID_PROFILE: {
            TyonProfile *p = (TyonProfile *) payload.constData();
#ifdef QT_DEBUG
            qDebug("[HIDDEV] PROFILE: ACTIVE_PROFILE=%d", p->profile_index);
#endif
            memcpy(&m_profile, p, sizeof(TyonProfile));
            profile = m_profiles[p->profile_index];
            m_profiles[profile.index] = profile;
            emit profileIndexChanged(p->profile_index);
            break;
        }
        case TYON_REPORT_ID_PROFILE_SETTINGS: {
            TyonProfileSettings *p = (TyonProfileSettings *) payload.constData();
            profile = m_profiles[p->profile_index];
            memcpy(&profile.settings, p, sizeof(TyonProfileSettings));
            m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
            debugSettings(profile, m_profile.profile_index);
#endif
            emit profileChanged(profile);
            break;
        }
        case TYON_REPORT_ID_PROFILE_BUTTONS: {
            TyonProfileButtons *p = (TyonProfileButtons *) payload.constData();
            profile = m_profiles[p->profile_index];
            memcpy(&profile.buttons, p, sizeof(TyonProfileButtons));
            m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
            debugButtons(profile, m_profile.profile_index);
#endif
            emit profileChanged(profile);

            // all profiles done...
            if (profile.index == TYON_PROFILE_NUM) {
                emit deviceFound();
            }
            break;
        }
        case TYON_REPORT_ID_MACRO: {
            TyonMacro *p = (TyonMacro *) payload.constData();
            if (p->count != sizeof(TyonMacro))
                break;
#ifdef QT_DEBUG
            qDebug("[HIDDEV] MACRO: #%02d group=%s name=%s", p->button_index, p->macroset_name, p->macro_name);
#else
            Q_UNUSED(p);
#endif
            break;
        }
        case TYON_REPORT_ID_DEVICE_STATE: {
            TyonDeviceState *p = (TyonDeviceState *) payload.constData();
            //#ifdef QT_DEBUG
            qDebug("[HIDDEV] STATE: state=%d", p->state);
            //#endif
            break;
        }
        default: {
            qDebug("[HIDDEV] RID UNHANDLED: rid=%d", rid);
        }
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::hidGetReportById(IOHIDDeviceRef device, int rid, CFIndex size)
{
    if (!size || !rid) {
        qCritical() << "[HIDDEV] Invalid parameters.";
        return EINVAL;
    }

    IOReturn ret;
    CFIndex length = size;
    quint8 *buffer = (quint8 *) malloc(length);
    memset(buffer, 0x00, length);

    ret = IOHIDDeviceGetReport(device, kIOHIDReportTypeFeature, rid, buffer, &length);
    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read HID device.");
        free(buffer);
        return EIO;
    }
    if (length == 0) {
        qWarning("[HIDDEV] Unexpected data length of HID report 0x%02x.", rid);
        free(buffer);
        return EIO;
    }

#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] GetReport(%p): [ID:0x%04x LEN:%lld] pl=%s", device, rid, d.length(), qPrintable(d.toHex(' ')));
#endif

    ret = parsePayload(rid, buffer, length);
    free(buffer);
    return ret;
}

inline int RTHidDevice::hidCheckWrite(IOHIDDeviceRef device)
{
    const IOHIDReportType hrt = toMacOSReportType(TYON_INTERFACE_MOUSE);

    quint8 rid = TYON_REPORT_ID_CONTROL;
    CFIndex length = sizeof(RoccatControl);
    RoccatControl *buffer = (RoccatControl *) malloc(length);

    memset(buffer, 0x00, length);

    IOReturn ret = kIOReturnSuccess;
    while (ret == kIOReturnSuccess) {
        ret = IOHIDDeviceGetReport(device, hrt, rid, (quint8 *) buffer, &length);
        if (ret != kIOReturnSuccess) {
            qCritical("[HIDDEV] Unable to read HID device.");
            goto func_exit;
        }
        if (length == 0) {
            qWarning("[HIDDEV] Unexpected data length of HID report 0x%02x.", rid);
            goto func_exit;
        }

#ifdef QT_DEBUG
        QByteArray d((char *) buffer, length);
        qDebug("[HIDDEV] hidCheckWrite(%p): [RID:0x%02x LEN:%lld] pl=%s", device, rid, d.length(), qPrintable(d.toHex(' ')));
#endif

        switch (buffer->value) {
            case ROCCAT_CONTROL_VALUE_STATUS_OK: {
                goto func_exit;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_BUSY: {
                QThread::msleep(500);
                break;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_1:
            case ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_2: {
                emit deviceError(buffer->value, tr("Got critical status"));
                ret = kIOReturnIOError;
                break;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_INVALID: {
                emit deviceError(buffer->value, tr("Got invalid status"));
                ret = kIOReturnIOError;
                break;
            }
            default: {
                emit deviceError(buffer->value, tr("Got unknown error"));
                ret = kIOReturnIOError;
                break;
            }
        }
    }

func_exit:
    free(buffer);
    return ret;
}

inline int RTHidDevice::hidWriteRoccatCtl(IOHIDDeviceRef device, uint pix, uint req)
{
    /* ROCCAT control message */
    RoccatControl control;
    control.report_id = TYON_REPORT_ID_CONTROL;
    control.value = pix;
    control.request = req;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(RoccatControl));
}

inline int RTHidDevice::hidSetReportRaw(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length)
{
    const IOHIDReportType hrt = toMacOSReportType(TYON_INTERFACE_MOUSE);
    const quint8 rid = buffer[0]; // required!

#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] hidSetReportRaw(%p): [RID:0x%02x LEN:%ld] pl=%s", device, rid, length, qPrintable(d.toHex(' ')));
#endif

    return hidWriteAsync(device, hrt, rid, buffer, length);
}

inline int RTHidDevice::hidWriteSync(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length)
{
    IOReturn ret = IOHIDDeviceSetReport(device, hrt, rid, buffer, length);
    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to write HID raw message. ret=0x%08x", ret);
        return ret;
    }
    QThread::msleep(250);
    return ret;
}

inline int RTHidDevice::hidWriteAsync(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length)
{
    const CFTimeInterval timeout = 4.0f;

    m_isCBComplete = false;

    IOReturn ret = IOHIDDeviceSetReportWithCallback(device, hrt, rid, buffer, length, timeout, _reportCallback, this);
    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to write HID report 0x%02lx.", rid);
        return ret;
    }

    auto checkComplete = [this]() -> bool {
        QMutexLocker lock(&m_mutex);
        return m_isCBComplete;
    };

    QDeadlineTimer dt(timeout * 1000, Qt::PreciseTimer);
    while (!checkComplete() && !dt.hasExpired()) {
        qApp->processEvents();
    }
    if (dt.hasExpired()) {
        qCritical("[HIDDEV] Timeout while waiting for HID device %p.", device);
        return kIOReturnTimeout;
    }

    return ret;
}
