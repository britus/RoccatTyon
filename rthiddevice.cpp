#include "rthiddevice.h"
#include "hid_uid.h"
#include "rttypes.h"
#include <DriverKit/IOUserClient.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <dispatch/dispatch.h>
#include <QColor>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeySequence>
#include <QMutexLocker>
#include <QRgb>
#include <QRgba64>
#include <QRgbaFloat16>
#include <QRgbaFloat32>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>

//#undef QT_DEBUG

#ifdef QT_DEBUG
#include "rthiddevicedbg.hpp"
#endif

Q_DECLARE_OPAQUE_POINTER(IOHIDDeviceRef)

// Not usable in case of TRAP on any 'registerCallback'
//#define MAC_HID_USE_DISPATCH_QUEUE

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
//
#ifdef QT_DEBUG
static inline void debugDevice(IOHIDDeviceRef device);
static inline void debugDevInfo(TyonInfo *p);
static inline void debugSettings(const RTHidDevice::TProfile profile, quint8 currentPix);
static inline void debugButtons(const RTHidDevice::TProfile profile, quint8 currentPix);
#endif

static inline void _deviceAttachedCallback(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
    if (!device || !context) {
        qCritical("[HIDDEV] _deviceAttachedCallback: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    debugDevice(device);
#endif

    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    ctx->onDeviceFound(device);
}

static inline void _deviceRemovedCallback(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
    if (!device || !context) {
        qCritical("[HIDDEV] _deviceRemovedCallback: NULL pointer in required parameters!");
        return;
    }

    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    ctx->onDeviceRemoved(device);
}

static inline void _inputReportCallback(void *context, IOReturn result, void *device, IOHIDReportType /*type*/, uint32_t reportID, uint8_t *report, CFIndex reportLength)
{
    if (!device || !context || !report) {
        qCritical("[HIDDEV] _deviceRemovedCallback: NULL pointer in required parameters!");
        return;
    }
    if (result != kIOReturnSuccess) {
        qCritical("[HIDDEV] _deviceRemovedCallback: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    qDebug("[HIDDEV] _inputReportCallback(%p): rid=0x%02x len=%ld result=0x%08x", device, reportID, reportLength, result);
#endif

    // X-Celerator calibration events
    if (reportID == TYON_REPORT_ID_SPECIAL) {
        RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
        ctx->onSpecialReport(reportID, reportLength, report);
    }
}

#if 0
static inline void _inputValueCallback(void *context, IOReturn result, void *device, IOHIDValueRef value)
{
    qDebug("[HIDDEV] _inputValueCallback: CTX=%p device=%p result=0x%08x value=%p", //
           context,
           device,
           result,
           value);
    // Get the associated element
    IOHIDElementRef element = IOHIDValueGetElement(value);
    if (element && !IOHIDElementHasNullState(element)) {
        qDebug("[HIDDEV] _inputValueCallback: element=%p", element);
        CFStringRef _name = IOHIDElementGetName(element);
        QString name = QString::fromCFString(_name);

        switch (IOHIDElementGetType(element)) {
            case kIOHIDElementTypeInput_Misc: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeInput_Misc: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeInput_Button: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeInput_Button: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeInput_Axis: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeInput_Axis: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeInput_ScanCodes: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeInput_ScanCodes: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeInput_NULL: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeInput_NULL: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeOutput: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeOutput: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeFeature: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeFeature: name=%s", qPrintable(name));
                break;
            }
            case kIOHIDElementTypeCollection: {
                qDebug("[HIDDEV] _inputValueCallback: kIOHIDElementTypeFeature: name=%s", qPrintable(name));
                break;
            }
        }
    }
}
#endif

// Callback for IOHIDDeviceSetReportWithCallback
static inline void _reportCallback(void *context, IOReturn result, void *device, IOHIDReportType type, uint32_t reportID, uint8_t *report, CFIndex reportLength)
{
    if (!report || !context) {
        qCritical("[HIDDEV] _deviceAttachedCallback: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    qDebug("[HIDDEV] _reportCallback: CTX=%p device=%p result=%d RT=%d RID=%d SIZE=%ld ", //
           context,
           device,
           result,
           type,
           reportID,
           reportLength);
#else
    Q_UNUSED(type)
#endif

    RTHidDevice *ctx = static_cast<RTHidDevice *>(context);
    ctx->onSetReport(result, reportID, reportLength, report);
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
    , m_wrkrDevices()
    , m_colors()
    , m_info()
    , m_profile()
    , m_profiles()
    , m_waitMutex()
    , m_accessMutex()
    , m_isCBComplete(false)
    , m_initComplete(false)
    , m_requestedProfile(0)
    , m_controlUnit()
    , m_sensor()
    , m_sensorImage()
    , m_hidQueue(0)
{
    qRegisterMetaType<IOHIDDeviceRef>();
    initializeColorMapping();
    initializeProfiles();
}

RTHidDevice::~RTHidDevice()
{
    internalSaveProfiles();
    releaseManager();
}

void RTHidDevice::lookupDevice()
{
    IOReturn result;

    m_initComplete = false;

    emit lookupStarted();

    // cleanup last findings
    releaseManager();

    // start from top
    m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!m_manager) {
        raiseError(kIOReturnIOError, tr("Failed to create HID manager."));
        return;
    }

    // Set up matching dictionary for Roccat Tyon devices
    CFMutableArrayRef filter = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

    // Constants for Roccat Tyon mouse Black and White variants
    const uint32_t vendorId = USB_DEVICE_ID_VENDOR_ROCCAT;
    const uint32_t productIds[2] =        //
        {USB_DEVICE_ID_ROCCAT_TYON_BLACK, //
         USB_DEVICE_ID_ROCCAT_TYON_WHITE};

    auto createMatching = [this](uint32_t vendorId, uint32_t productId) -> CFMutableDictionaryRef {
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable( //
            kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        if (!dict) {
            raiseError(kIOReturnIOError, tr("Failed to create dictionary."));
            return nullptr;
        }

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
        if (!(dict = createMatching(vendorId, productId))) {
            CFRelease(m_manager);
            return;
        }
        CFArrayAppendValue(filter, dict);
        CFRelease(dict);
    }

    IOHIDManagerSetDeviceMatchingMultiple(m_manager, filter);
    CFRelease(filter);

    // Register for device attached and removed callbacks and schedule in runloop
    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, _deviceAttachedCallback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, _deviceRemovedCallback, this);

#ifdef MAC_HID_USE_DISPATCH_QUEUE

    dispatch_block_flags_t flags = {};
    dispatch_block_t cancelHandler = dispatch_block_create(flags, ^{
        if (m_manager) {
            qDebug("[HIDDEV] !!!! cancelHandler - FREE m_manager !!!!");
            IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
            CFRelease(m_manager);
            m_manager = nullptr;
        }
    });
    IOHIDManagerSetCancelHandler(m_manager, cancelHandler);

    // Set up the dispatch queue
    m_hidQueue = dispatch_queue_create("org.eof.tools.RoccatTyon", DISPATCH_QUEUE_SERIAL);
    if (!m_hidQueue) {
        raiseError(kIOReturnNoSpace, tr("Failed to create dispatch queue."));
        CFRelease(m_manager);
        m_manager = nullptr;
        return;
    }

    IOHIDManagerSetDispatchQueue(m_manager, m_hidQueue);
    IOHIDManagerActivate(m_manager);

#else //--mutual exlusive ^^
    IOHIDManagerScheduleWithRunLoop(m_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
#endif

    // Open HID manager -536870174
    // kernel[0:1b4e77] (IOHIDFamily) IOHIDLibUserClient:0x0
    // [RoccatTyon] Entitlements 0 privilegedClient : No
    result = IOHIDManagerOpen(m_manager, kIOHIDOptionsTypeSeizeDevice);
    if (result != kIOReturnSuccess && result != -536870174) {
        raiseError(result, tr("Failed to open HID manager."));
        CFRelease(m_manager);
        m_manager = nullptr;
        return;
    }
}

void RTHidDevice::resetProfiles()
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
    foreach (IOHIDDeviceRef device, m_wrkrDevices) {
        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            goto func_exit;
        }
        if ((ret = hidSetReportRaw(device, buffer, info.size)) != kIOReturnSuccess) {
            goto func_exit;
        }
    }

func_exit:
    if (ret != kIOReturnSuccess) {
        raiseError(ret,
                   tr("Unable to reset device profiles.\n"
                      "Please close the application and try again."));
    }

    QThread::msleep(1000);
    releaseManager();
    lookupDevice();
}

void RTHidDevice::saveProfilesToDevice()
{
    emit deviceWorkerStarted();

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
        foreach (IOHIDDeviceRef device, m_wrkrDevices) {
            if ((ret = writeIndex(device)) != kIOReturnSuccess) {
                goto thread_exit;
            }
            foreach (TProfile p, m_profiles) {
                if (p.changed) {
                    if ((ret = writeProfile(device, p)) != kIOReturnSuccess) {
                        goto thread_exit;
                    }
                    updateProfileMap(&p, false);
                }
            }
            if ((ret = talkWriteFxState(device, m_talkFx.fx_status)) != kIOReturnSuccess) {
                goto thread_exit;
            }
            if (m_controlUnit.tcu == TYON_TRACKING_CONTROL_UNIT_OFF) {
                if ((ret = tcuWriteOff(device, (TyonControlUnitDcu) m_controlUnit.dcu)) != kIOReturnSuccess) {
                    goto thread_exit;
                }
            } else {
                if ((ret = tcuWriteAccept(device, (TyonControlUnitDcu) m_controlUnit.dcu, m_controlUnit.median)) != kIOReturnSuccess) {
                    goto thread_exit;
                }
            }
        }
    thread_exit:
        t->exit(ret);
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

void RTHidDevice::saveProfilesToFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::Truncate | QFile::WriteOnly)) {
        emit deviceError(EIO, tr("Unable to save file: %1").arg(fileName));
        return;
    }

    emit deviceWorkerStarted();

    auto write = [](QFile *f, const void *p, const qsizetype size) -> IOReturn {
        if (f->write((char *) p, size) != size) {
            return kIOReturnIOError;
        }
        return kIOReturnSuccess;
    };

    quint32 length;
    foreach (const TProfile p, m_profiles) {
        if (p.name.length() == 0 || p.name.length() > HIDAPI_MAX_STR) {
            raiseError(kIOReturnInvalid, "Invalid profile name.");
            goto error_exit;
        }
        if (p.index >= TYON_PROFILE_NUM) {
            raiseError(kIOReturnInvalid, "Invalid profile index.");
            goto error_exit;
        }
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

void RTHidDevice::loadProfilesFromFile(const QString &fileName, bool raiseEvents)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        emit deviceError(EIO, tr("Unable to load file: %1").arg(fileName));
        return;
    }

    emit deviceWorkerStarted();

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
                updateProfileMap(profile, true);
                SafeDelete(profile);
                profile = 0L;
                pfcount++;
                stage = 0; // next profile
                break;
            }
        }
    } // for

func_exit:
    if (!raiseEvents) {
        blockSignals(false);
    }
    f.close();

    emit deviceWorkerFinished();
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

    const TUidToQtKeyMap *keymap = nullptr;
    quint8 mods = 0;

    if (func == TYON_BUTTON_TYPE_SHORTCUT) {
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

    TProfile p = m_profiles[profileIndex()];
    RoccatButton *b = &p.buttons.buttons[type];
    b->type = func;
    b->modifier = (keymap != nullptr ? mods : 0);
    b->key = (keymap != nullptr ? keymap->uid_key : 0);
    updateProfileMap(&p, true);
}

const QKeySequence RTHidDevice::toKeySequence(const RoccatButton &b) const
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

    const TUidToQtKeyMap *keymap = nullptr;
    for (const TUidToQtKeyMap *p = uid_2_qtkey; p->uid_key && p->qt_key != Qt::Key_unknown; p++) {
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
            if (p.name.isEmpty()) {
                raiseError(kIOReturnInvalid, "Invalid profile name.");
                return;
            }
            p.name = name;
            updateProfileMap(&p, true);
        }
    }
}

void RTHidDevice::setXSensitivity(qint16 sensitivity)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.sensitivity_x != sensitivity) {
            p.settings.sensitivity_x = sensitivity + ROCCAT_SENSITIVITY_CENTER;
            updateProfileMap(&p, true);
        }
    }
}

void RTHidDevice::setYSensitivity(qint16 sensitivity)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.sensitivity_y != sensitivity) {
            p.settings.sensitivity_y = sensitivity + ROCCAT_SENSITIVITY_CENTER;
            updateProfileMap(&p, true);
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
        updateProfileMap(&p, true);
    }
}

void RTHidDevice::setPollRate(quint8 rate)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.talkfx_polling_rate != rate) {
            p.settings.talkfx_polling_rate = rate;
            updateProfileMap(&p, true);
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
        updateProfileMap(&p, true);
    }
}

void RTHidDevice::setActiveDpiSlot(quint8 id)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.cpi_active != id) {
            p.settings.cpi_active = id;
            updateProfileMap(&p, true);
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
            updateProfileMap(&p, true);
        }
    }
}

void RTHidDevice::setLightsEffect(quint8 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.light_effect != value) {
            p.settings.light_effect = value;
            updateProfileMap(&p, true);
        }
    }
}

void RTHidDevice::setColorFlow(quint8 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.color_flow != value) {
            p.settings.color_flow = value;
            updateProfileMap(&p, true);
        }
    }
}

void RTHidDevice::setTalkFxState(bool state)
{
    m_talkFx.fx_status = (state ? ROCCAT_TALKFX_STATE_ON : ROCCAT_TALKFX_STATE_OFF);
}

void RTHidDevice::setDcuState(TyonControlUnitDcu state)
{
    m_controlUnit.dcu = state;
}

void RTHidDevice::setTcuState(TyonControlUnitTcu state)
{
    m_controlUnit.tcu = state;
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
        updateProfileMap(&p, true);
    }
}

TyonLight RTHidDevice::toDeviceColor(TyonLightType target, const QColor &color) const
{
    if ((quint8) target >= TYON_LIGHTS_NUM) {
        qWarning("[HIDDEV] toDeviceColor(): Invalid light index. 0 or 1 expected.");
        return {};
    }
    const TProfile p = m_profiles[profileIndex()];
    const TyonLight tl = p.settings.lights[target];
    TyonLight info = {};
    info.index = tl.index;
    info.unused = tl.unused;
    info.red = color.red();
    info.green = color.green();
    info.blue = color.blue();
    return info;
}

QColor RTHidDevice::toScreenColor(const TyonLight &light, bool isCustom) const
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

void RTHidDevice::setLightColor(TyonLightType target, const TyonLight &color)
{
    if ((quint8) target >= TYON_LIGHTS_NUM) {
        qWarning("[HIDDEV] Invalid light index. 0 or 1 expected.");
        return;
    }

    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
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
            updateProfileMap(&p, true);
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

bool RTHidDevice::talkFxState() const
{
    return m_talkFx.fx_status == ROCCAT_TALKFX_STATE_ON;
}

TyonControlUnitDcu RTHidDevice::dcuState() const
{
    return static_cast<TyonControlUnitDcu>(m_controlUnit.dcu);
}

TyonControlUnitTcu RTHidDevice::tcuState() const
{
    return static_cast<TyonControlUnitTcu>(m_controlUnit.tcu);
}

uint RTHidDevice::tcuMedian() const
{
    return m_controlUnit.median;
}

quint8 RTHidDevice::minimumXCelerate() const
{
    return m_info.xcelerator_min;
}

quint8 RTHidDevice::maximumXCelerate() const
{
    return m_info.xcelerator_max;
}

quint8 RTHidDevice::middleXCelerate() const
{
    return m_info.xcelerator_mid;
}

void RTHidDevice::xcStartCalibration()
{
    if (m_wrkrDevices.isEmpty()) {
        emit deviceError(kIOReturnNoDevice, "No ROCCAT Tyon device found.");
        return;
    }

    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t]() { //
        IOReturn ret;
        foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
            ret = xcCalibWriteStart(devices);
            if (ret != kIOReturnSuccess) {
                goto func_exit;
            }
        }
    func_exit:
        t->exit(ret);
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

void RTHidDevice::xcStopCalibration()
{
    if (m_wrkrDevices.isEmpty()) {
        emit deviceError(kIOReturnNoDevice, "No ROCCAT Tyon device found.");
        return;
    }

    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t]() { //
        IOReturn ret;
        foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
            ret = xcCalibWriteEnd(devices);
            if (ret != kIOReturnSuccess) {
                goto func_exit;
            }
        }
    func_exit:
        t->exit(ret);
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

void RTHidDevice::tcuSensorTest(TyonControlUnitDcu dcu, uint median)
{
    IOReturn ret;
    foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
        ret = tcuWriteTest(devices, dcu, median);
        if (ret != kIOReturnSuccess) {
            break;
        }
    }
}

void RTHidDevice::tcuSensorAccept(TyonControlUnitDcu dcuState, uint median)
{
    IOReturn ret;
    foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
        ret = tcuWriteAccept(devices, dcuState, median);
        if (ret != kIOReturnSuccess) {
            break;
        }
    }
}

void RTHidDevice::tcuSensorCancel(TyonControlUnitDcu dcuState)
{
    IOReturn ret;
    foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
        ret = tcuWriteCancel(devices, dcuState);
        if (ret != kIOReturnSuccess) {
            break;
        }
    }
}

void RTHidDevice::tcuSensorCaptureImage()
{
    IOReturn ret;
    foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
        ret = sensorCaptureImage(devices);
        if (ret != kIOReturnSuccess) {
            break;
        }
    }
}

void RTHidDevice::tcuSensorReadImage()
{
    IOReturn ret;
    foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
        ret = sensorReadImage(devices);
        if (ret != kIOReturnSuccess) {
            break;
        }
    }
}

int RTHidDevice::tcuSensorReadMedian(TyonSensorImage *image)
{
    return sensorMedianOfImage(image);
}

void RTHidDevice::xcApplyCalibration(quint8 min, quint8 mid, quint8 max)
{
    if (m_wrkrDevices.isEmpty()) {
        emit deviceError(kIOReturnNoDevice, "No ROCCAT Tyon device found.");
        return;
    }

    emit deviceWorkerStarted();

    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, min, mid, max]() { //
        IOReturn ret;
        foreach (IOHIDDeviceRef devices, m_wrkrDevices) {
            qInfo("[HIDDEV] Apply X-Celerator min=%d mid=%d max=%d", min, mid, max);
            ret = xcCalibWriteData(devices, min, mid, max);
            if (ret != kIOReturnSuccess) {
                goto func_exit;
            }
        }
    func_exit:
        t->exit(ret);
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

void RTHidDevice::onDeviceFound(IOHIDDeviceRef device)
{
    QThread *t = new QThread();
    connect(t, &QThread::started, this, [this, t, device]() {
        IOReturn ret = kIOReturnSuccess;

        /* skip if already read */
        if (m_wrkrDevices.contains(device)) {
            goto func_exit;
        }

        // some devices cannot open ?? 4 times called for product
        ret = IOHIDDeviceOpen(device, kIOHIDOptionsTypeSeizeDevice);
        if (ret != kIOReturnSuccess) {
            qDebug("[HIDDEV] Unable to open device: %p", device);
            goto func_exit;
        }

#ifdef QT_DEBUG
        qDebug("[HIDDEV] Device(%p) opened", device);
#endif
        /* read device control state */
        if ((ret = readDeviceControl(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read device control state.");
            goto error_exit;
        }

        /* read device control unit */
        if ((ret = readDeviceControlUnit(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read device control unit.");
            goto error_exit;
        }

        /* read talk-fx status */
        if ((ret = readDeviceTalk(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read device Talk-FX status.");
            goto error_exit;
        }

        /* read firmware release */
        if ((ret = readDeviceInfo(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read device info.");
            goto error_exit;
        }

        /* read current profile number */
        if ((ret = readActiveProfile(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read device profile.");
            goto error_exit;
        }

        /* read profile slots */
        for (quint8 pix = 0; pix < TYON_PROFILE_NUM; pix++) {
            if ((ret = readProfile(device, pix)) != kIOReturnSuccess) {
                // device does not response to requested profile index.
                // responsed profile.settings.profile_index differ from pix
                // skip this silent...
                if (ret == kIOReturnNotAttached) {
                    qInfo("[HIDDEV] Device '%p' isn't control. Set as misc device.", device);
                    goto skip_exit;
                }
                raiseError(ret, "Unable to read device profile.");
                goto error_exit;
            }
        }

        /* read control unit */
        if ((ret = readDeviceControlUnit(device)) != kIOReturnSuccess) {
            raiseError(ret, "Unable to read control unit.");
            goto error_exit;
        }

        // succes append for usage
        m_wrkrDevices.append(device);
        emit deviceFound();
        goto func_exit;

    skip_exit:
        m_miscDevices.append(device);
        // register device callbacks for any reports
        IOHIDDeviceRegisterInputReportCallback(device, m_cbReportBuffer, m_cbReportLength, _inputReportCallback, this);
        //IOHIDDeviceRegisterInputValueCallback(device, _inputValueCallback, this);
        return;

    error_exit:
        IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
        //ret = raiseError(kIOReturnIOError, "Unable to load device data.");

    func_exit:
        t->exit(ret);
    });
    connect(t, &QThread::finished, this, [t]() { //
        t->deleteLater();
    });
    connect(t, &QThread::destroyed, this, [](QObject *) {});
    t->start(QThread::IdlePriority);
    QThread::yieldCurrentThread();
}

void RTHidDevice::onDeviceRemoved(IOHIDDeviceRef device)
{
    if (m_wrkrDevices.contains(device)) {
        m_wrkrDevices.removeOne(device);
    }
    if (m_miscDevices.contains(device)) {
        m_miscDevices.removeOne(device);
    }
    emit deviceRemoved();
}

void RTHidDevice::onSetReport(IOReturn status, uint rid, CFIndex length, uint8_t *report)
{
    QMutexLocker lock(&m_waitMutex);
    m_isCBComplete = true;
#ifdef QT_DEBUG
    if (length > 0) {
        const QByteArray data((char *) report, length);
        qDebug("[HIDDEV] status=%d RID=0x%02x SIZE=%ld", status, rid, length);
        qDebug("[HIDDEV] data=%s", qPrintable(data.toHex(' ')));
    }
#else
    Q_UNUSED(status)
    Q_UNUSED(rid)
    Q_UNUSED(index)
    Q_UNUSED(length)
    Q_UNUSED(report)
#endif
}

void RTHidDevice::onSpecialReport(uint rid, CFIndex length, uint8_t *report)
{
    const QByteArray payload((char *) report, length);
    emit specialReport(rid, payload);
}

inline int RTHidDevice::readProfile(IOHIDDeviceRef device, quint8 pix)
{
    IOReturn ret;

    /* select profile settings store */
    if ((ret = selectProfileSettings(device, pix)) != kIOReturnSuccess) {
        emit deviceError(ret, "Unable to select profile.");
        return ret;
    }

    /* read profile settings */
    if ((ret = readProfileSettings(device)) != kIOReturnSuccess) {
        /* requested profile settings not arrived, skip. */
        if (ret == kIOReturnNotAttached) {
            return ret;
        }
        emit deviceError(ret, "Unable to read profile settings.");
        return ret;
    }

    /* select profile buttons store */
    if ((ret = selectProfileButtons(device, pix)) != kIOReturnSuccess) {
        emit deviceError(ret, "Unable to select profile buttons.");
        return ret;
    }

    /* read profile buttons */
    if ((ret = readProfileButtons(device)) != kIOReturnSuccess) {
        /* requested profile button not arrived, skip. */
        if (ret == kIOReturnNotAttached) {
            return ret;
        }
        emit deviceError(ret, "Unable to read profile buttons.");
        return ret;
    }

#if 0
    /* read all button slots including combined with EasyShift */
    for (quint8 bix = 0; bix < TYON_PROFILE_BUTTON_NUM; bix++) {
        if ((ret = readButtonMacro(device, pix, bix)) != kIOReturnSuccess) {
            if (ret != ENODATA) { // optional macros
                emit deviceError(ret, "Unable to read profile macro.");
                return ret;
            }
        }
    }
#endif

    // reset change flag
    setModified(pix, false);
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
    //-
    while (!m_wrkrDevices.isEmpty()) {
        if ((dev = m_wrkrDevices.takeFirst())) {
            IOHIDDeviceClose(dev, kIOHIDOptionsTypeSeizeDevice);
        }
    }
    m_wrkrDevices.clear();
    //-
    while (!m_miscDevices.isEmpty()) {
        if ((dev = m_miscDevices.takeFirst())) {
            IOHIDDeviceClose(dev, kIOHIDOptionsTypeSeizeDevice);
        }
    }
    m_miscDevices.clear();
}

inline void RTHidDevice::releaseManager()
{
    releaseDevices();

    if (m_manager) {
#ifdef MAC_HID_USE_DISPATCH_QUEUE
        qDebug("[HIDDEV] call IOHIDManagerCancel !!!!");
        IOHIDManagerCancel(m_manager);
#else // ^^ and below mutual exclusive (DOUBLE FREE)
        IOHIDManagerUnscheduleFromRunLoop(m_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
        CFRelease(m_manager);
        m_manager = nullptr;
#endif
    }
    if (m_hidQueue) {
        dispatch_release(m_hidQueue);
        m_hidQueue = nullptr;
    }
}

inline int RTHidDevice::raiseError(int error, const QString &message)
{
    qCritical("[HIDDEV] Error 0x%08x: %s", error, qPrintable(message));
    emit deviceError(error, message);
    return error;
}

inline void RTHidDevice::initializeColorMapping()
{
    for (quint8 i = 0; i < TYON_LIGHT_INFO_COLORS_NUM; i++) {
        m_colors[i].deviceColors = roccat_colors[i];
    }
}

inline void RTHidDevice::initializeProfiles()
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

inline void RTHidDevice::internalSaveProfiles()
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

inline void RTHidDevice::setModified(quint8 pix, bool changed)
{
    if (m_profiles.contains(pix)) {
        TProfile p = m_profiles[pix];
        updateProfileMap(&p, changed);
    }
}

inline void RTHidDevice::setModified(TProfile *p, bool changed)
{
    if (p->changed != changed) {
        p->changed = changed;
    }
}

inline void RTHidDevice::updateProfileMap(TProfile *p, bool changed)
{
    if (p->index >= TYON_PROFILE_NUM) {
        raiseError(kIOReturnInvalid, "Invalid profile index.");
        return;
    }

    setModified(p, changed);
    m_profiles[p->index] = (*p);

    // emit event only on active profile
    if (p->index == profileIndex()) {
        emit profileChanged((*p));
    }
}

inline int RTHidDevice::readDeviceSpecial(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_SPECIAL, 256);
}

inline int RTHidDevice::readDeviceControl(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_CONTROL, sizeof(RoccatControl));
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

inline int RTHidDevice::readDeviceTalk(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_TALK, sizeof(TyonTalk));
}

inline int RTHidDevice::readDeviceControlUnit(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_CONTROL_UNIT, sizeof(TyonControlUnit));
}

inline int RTHidDevice::readDevice0A(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_A, 8);
}

inline int RTHidDevice::readDevice11(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_11, 16);
}

inline int RTHidDevice::readDevice1A(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_1A, 1029);
}

inline int RTHidDevice::tcuWriteTest(IOHIDDeviceRef device, TyonControlUnitDcu dcuState, uint median)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control.median = median;
    control.action = TYON_CONTROL_UNIT_ACTION_CANCEL;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::tcuWriteAccept(IOHIDDeviceRef device, TyonControlUnitDcu dcuState, uint median)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control.median = median;
    control.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::tcuWriteOff(IOHIDDeviceRef device, TyonControlUnitDcu dcuState)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = TYON_TRACKING_CONTROL_UNIT_OFF;
    control.median = 0;
    control.action = TYON_CONTROL_UNIT_ACTION_OFF;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::tcuWriteTry(IOHIDDeviceRef device, TyonControlUnitDcu dcuState)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_UNDEFINED;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::tcuWriteCancel(IOHIDDeviceRef device, TyonControlUnitDcu dcuState)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_CANCEL;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::dcuWriteState(IOHIDDeviceRef device, TyonControlUnitDcu dcuState)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonControlUnit control;
    control.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control.size = sizeof(TyonControlUnit);
    control.dcu = dcuState;
    control.tcu = 0xff;
    control.median = 0xff;
    control.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;
    return hidSetReportRaw(device, (const quint8 *) &control, sizeof(TyonControlUnit));
}

inline int RTHidDevice::sensorRead(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_SENSOR, sizeof(TyonSensor));
}

inline int RTHidDevice::sensorReadImage(IOHIDDeviceRef device)
{
    return hidGetReportById(device, TYON_REPORT_ID_SENSOR, sizeof(TyonSensorImage));
}

inline int RTHidDevice::sensorReadRegister(IOHIDDeviceRef device, quint8 reg)
{
    IOReturn ret;

    ret = sensorWriteStruct(device, TYON_SENSOR_ACTION_READ, reg, 0);
    if (ret != kIOReturnSuccess) {
        return ret;
    }

    return sensorRead(device);
}

inline int RTHidDevice::sensorWriteStruct(IOHIDDeviceRef device, quint8 action, quint8 reg, quint8 value)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonSensor sensor;
    sensor.report_id = TYON_REPORT_ID_SENSOR;
    sensor.action = action;
    sensor.reg = reg;
    sensor.value = value;

    const IOHIDReportType hrt = toMacOSReportType(TYON_INTERFACE_MOUSE);
    if ((ret = hidWriteReport(device, //
                              hrt,
                              sensor.report_id,
                              (const quint8 *) &sensor,
                              sizeof(TyonSensor)))
        != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to write sensor command."));
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::sensorCaptureImage(IOHIDDeviceRef device)
{
    return sensorWriteStruct(device, TYON_SENSOR_ACTION_FRAME_CAPTURE, 1, 0);
}

inline int RTHidDevice::sensorWriteRegister(IOHIDDeviceRef device, quint8 reg, quint8 value)
{
    return sensorWriteStruct(device, TYON_SENSOR_ACTION_WRITE, reg, value);
}

uint RTHidDevice::sensorMedianOfImage(TyonSensorImage const *image)
{
    uint i;
    ulong sum = 0;
    for (i = 0; i < TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE; ++i)
        sum += image->data[i];
    return sum / (TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE);
}

inline int RTHidDevice::xcCalibWriteStart(IOHIDDeviceRef device)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_START;
    IOHIDReportType hrt = kIOHIDReportTypeFeature;
    return hidWriteReport(device, hrt, info.report_id, (const quint8 *) &info, sizeof(TyonInfo));
}

inline int RTHidDevice::xcCalibWriteEnd(IOHIDDeviceRef device)
{
    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_END;
    IOHIDReportType hrt = kIOHIDReportTypeFeature;

    return hidWriteReport(device, hrt, info.report_id, (const quint8 *) &info, sizeof(TyonInfo));
}

inline int RTHidDevice::xcCalibWriteData(IOHIDDeviceRef device, quint8 min, quint8 mid, quint8 max)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_DATA;
    info.xcelerator_min = min;
    info.xcelerator_mid = mid;
    info.xcelerator_max = max;

    return hidSetReportRaw(device, (const quint8 *) &info, sizeof(TyonInfo));
}

inline int RTHidDevice::readButtonMacro(IOHIDDeviceRef device, uint pix, uint bix)
{
    IOReturn ret;

    if (pix >= TYON_PROFILE_NUM) {
        return raiseError(kIOReturnBadArgument, tr("Invalid profile index parameter."));
    }

    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        return raiseError(kIOReturnBadArgument, tr("Invalid button index parameter."));
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
        return raiseError(kIOReturnBadArgument, tr("Invalid profile index."));
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_SETTINGS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    m_requestedProfile = pix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile settings. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

inline int RTHidDevice::selectProfileButtons(IOHIDDeviceRef device, uint pix)
{
    if (pix >= TYON_PROFILE_NUM) {
        return raiseError(kIOReturnBadArgument, tr("Invalid profile index."));
    }

    const quint8 _req = TYON_CONTROL_REQUEST_PROFILE_BUTTONS;
    const quint8 _dix = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    m_requestedProfile = pix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile buttons. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

inline int RTHidDevice::selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix)
{
    if (pix >= TYON_PROFILE_NUM) {
        return raiseError(kIOReturnBadArgument, tr("Invalid profile index."));
    }
    if (bix >= TYON_PROFILE_BUTTON_NUM) {
        return raiseError(kIOReturnBadArgument, tr("Invalid macro index."));
    }

    const quint8 _dix = (dix | pix);
    const quint8 _req = bix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select macro. PIX=%d DIX=0x%02x REQ=0x%02x", pix, _dix, _req);
#endif

    return hidWriteRoccatCtl(device, _dix, _req);
}

inline int RTHidDevice::talkWriteReport(IOHIDDeviceRef device, TyonTalk *talk)
{
    IOReturn ret;

    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

    talk->report_id = TYON_REPORT_ID_TALK;
    talk->size = sizeof(TyonTalk);
    return hidSetReportRaw(device, (const quint8 *) &talk, sizeof(TyonTalk));
}

inline int RTHidDevice::talkWriteKey(IOHIDDeviceRef device, quint8 easyshift, quint8 easyshift_lock, quint8 easyaim)
{
    TyonTalk talk;

    memset(&talk, 0, sizeof(TyonTalk));

    talk.easyshift = easyshift;
    talk.easyshift_lock = easyshift_lock;
    talk.easyaim = easyaim;
    talk.fx_status = TYON_TALKFX_STATE_UNUSED;

    return talkWriteReport(device, &talk);
}

inline int RTHidDevice::talkWriteEasyshift(IOHIDDeviceRef device, quint8 state)
{
    return talkWriteKey(device, state, TYON_TALK_EASYSHIFT_UNUSED, TYON_TALK_EASYAIM_UNUSED);
}

inline int RTHidDevice::talkWriteEasyshiftLock(IOHIDDeviceRef device, quint8 state)
{
    return talkWriteKey(device, TYON_TALK_EASYSHIFT_UNUSED, state, TYON_TALK_EASYAIM_UNUSED);
}

inline int RTHidDevice::talkWriteEasyAim(IOHIDDeviceRef device, quint8 state)
{
    return talkWriteKey(device, TYON_TALK_EASYSHIFT_UNUSED, TYON_TALK_EASYSHIFT_UNUSED, state);
}

inline int RTHidDevice::talkWriteFxData(IOHIDDeviceRef device, TyonTalk *talk)
{
    talk->easyshift = TYON_TALK_EASYSHIFT_UNUSED;
    talk->easyshift_lock = TYON_TALK_EASYSHIFT_UNUSED;
    talk->easyaim = TYON_TALK_EASYAIM_UNUSED;
    return talkWriteReport(device, talk);
}

inline int RTHidDevice::talkWriteFx(IOHIDDeviceRef device, quint32 effect, quint32 ambient_color, quint32 event_color)
{
    TyonTalk talk;
    uint zone;

    memset(&talk, 0, sizeof(TyonTalk));

    talk.fx_status = ROCCAT_TALKFX_STATE_ON;

    zone = (effect & ROCCAT_TALKFX_ZONE_BIT_MASK) >> ROCCAT_TALKFX_ZONE_BIT_SHIFT;
    talk.zone = (zone == ROCCAT_TALKFX_ZONE_AMBIENT) ? TYON_TALKFX_ZONE_AMBIENT : TYON_TALKFX_ZONE_EVENT;

    talk.effect = (effect & ROCCAT_TALKFX_EFFECT_BIT_MASK) >> ROCCAT_TALKFX_EFFECT_BIT_SHIFT;
    talk.speed = (effect & ROCCAT_TALKFX_SPEED_BIT_MASK) >> ROCCAT_TALKFX_SPEED_BIT_SHIFT;
    talk.ambient_red = (ambient_color & ROCCAT_TALKFX_COLOR_RED_MASK) >> ROCCAT_TALKFX_COLOR_RED_SHIFT;
    talk.ambient_green = (ambient_color & ROCCAT_TALKFX_COLOR_GREEN_MASK) >> ROCCAT_TALKFX_COLOR_GREEN_SHIFT;
    talk.ambient_blue = (ambient_color & ROCCAT_TALKFX_COLOR_BLUE_MASK) >> ROCCAT_TALKFX_COLOR_BLUE_SHIFT;
    talk.event_red = (event_color & ROCCAT_TALKFX_COLOR_RED_MASK) >> ROCCAT_TALKFX_COLOR_RED_SHIFT;
    talk.event_green = (event_color & ROCCAT_TALKFX_COLOR_GREEN_MASK) >> ROCCAT_TALKFX_COLOR_GREEN_SHIFT;
    talk.event_blue = (event_color & ROCCAT_TALKFX_COLOR_BLUE_MASK) >> ROCCAT_TALKFX_COLOR_BLUE_SHIFT;

    return talkWriteFxData(device, &talk);
}

inline int RTHidDevice::talkWriteFxState(IOHIDDeviceRef device, quint8 state)
{
    TyonTalk talk;
    memset(&talk, 0, sizeof(TyonTalk));
    talk.fx_status = state;
    return talkWriteFxData(device, &talk);
}

// Read ROCCAT Mouse report descriptor
inline int RTHidDevice::hidParseResponse(int rid, const quint8 *buffer, CFIndex)
{
    IOReturn ret = kIOReturnSuccess;
    TProfile profile = {};
    switch (rid) {
        case TYON_REPORT_ID_CONTROL: {
            RoccatControl *p = (RoccatControl *) buffer;
            //#ifdef QT_DEBUG
            qDebug("[HIDDEV] TYON_REPORT_ID_CONTROL: reqest=0x%02x value=0x%02x", p->request, p->value);
            //#endif
            break;
        }
        case TYON_REPORT_ID_INFO: {
            TyonInfo *p = (TyonInfo *) buffer;
            memcpy(&m_info, p, sizeof(TyonInfo));
#ifdef QT_DEBUG
            debugDevInfo(&m_info);
#endif
            emit deviceInfo(m_info);
            break;
        }
        case TYON_REPORT_ID_PROFILE: {
            TyonProfile *p = (TyonProfile *) buffer;
#ifdef QT_DEBUG
            qDebug("[HIDDEV] PROFILE: ACTIVE_PROFILE=%d", p->profile_index);
#endif
            memcpy(&m_profile, p, sizeof(TyonProfile));
            profile = m_profiles[p->profile_index];
            m_profiles[profile.index] = profile;
            emit profileIndexChanged(p->profile_index);
            break;
        }
        // called for each profile slot
        case TYON_REPORT_ID_PROFILE_SETTINGS: {
            TyonProfileSettings *p = (TyonProfileSettings *) buffer;
            // device seems not to be responsible for requested
            // profile index this skip silent...
            if (p->profile_index != m_requestedProfile) {
                ret = kIOReturnNotAttached;
                goto func_exit;
            }
            profile = m_profiles[p->profile_index];
            memcpy(&profile.settings, p, sizeof(TyonProfileSettings));
            m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
            debugSettings(profile, m_profile.profile_index);
#endif
            emit profileChanged(profile);
            break;
        }
        // called for each profile slot
        case TYON_REPORT_ID_PROFILE_BUTTONS: {
            TyonProfileButtons *p = (TyonProfileButtons *) buffer;
            // device seems not to be responsible for requested
            // profile index skip silent...
            if (p->profile_index != m_requestedProfile) {
                ret = kIOReturnNotAttached;
                goto func_exit;
            }
            profile = m_profiles[p->profile_index];
            memcpy(&profile.buttons, p, sizeof(TyonProfileButtons));
            m_profiles[profile.index] = profile;
#ifdef QT_DEBUG
            debugButtons(profile, m_profile.profile_index);
#endif
            emit profileChanged(profile);
            break;
        }
        case TYON_REPORT_ID_MACRO: {
            TyonMacro *p = (TyonMacro *) buffer;
            if (p->count != sizeof(TyonMacro)) {
                break;
            }
#ifdef QT_DEBUG
            qDebug("[HIDDEV] MACRO: #%02d group=%s name=%s", p->button_index, p->macroset_name, p->macro_name);
#else
            Q_UNUSED(p);
#endif
            break;
        }
        case TYON_REPORT_ID_DEVICE_STATE: {
            TyonDeviceState *p = (TyonDeviceState *) buffer;
            //#ifdef QT_DEBUG
            qDebug("[HIDDEV] DEVICE_STATE: state=%d", p->state);
            //#endif
            break;
        }
        /* Tracking control unit (TCU) calibration events */
        case TYON_REPORT_ID_CONTROL_UNIT: {
            TyonControlUnit *p = (TyonControlUnit *) buffer;
            //#ifdef QT_DEBUG
            qDebug("[HIDDEV] CONTROL_UNIT: action=0x%02x dcu=%d tcu=%d median=%d", p->action, p->dcu, p->tcu, p->median);
            //#endif
            memcpy(&m_controlUnit, p, sizeof(TyonControlUnit));
            emit controlUnitChanged(m_controlUnit);
            break;
        }
        case TYON_REPORT_ID_SENSOR: {
            TyonSensor *p = (TyonSensor *) buffer;
            //#ifdef QT_DEBUG
            qDebug("[HIDDEV] SENSOR: action=%d reg=%d value=%d", p->action, p->reg, p->value);
            //#endif
            if (p->action == 3 && p->value == 0) {
                memcpy(&m_sensor, p, sizeof(TyonSensor));
                emit sensorChanged(m_sensor);
            } else {
                memcpy(&m_sensorImage, p, sizeof(TyonSensorImage));
                emit sensorImageChanged(m_sensorImage);
            }
            break;
        }
        /* X-Celerator calibration events */
        case TYON_REPORT_ID_SPECIAL: {
#ifdef QT_DEBUG
            qDebug("[HIDDEV] TYON_REPORT_ID_SPECIAL: rid=%d", rid);
#endif
            break;
        }
        /* Talk-FX */
        case TYON_REPORT_ID_TALK: {
            TyonTalk *p = (TyonTalk *) buffer;
            memcpy(&m_talkFx, p, sizeof(TyonTalk));
            emit talkFxChanged(m_talkFx);
        }
        default: {
            qDebug("[HIDDEV] RID UNHANDLED: rid=%d", rid);
            break;
        }
    }

func_exit:
    qApp->processEvents();
    return ret;
}

inline int RTHidDevice::hidGetReportById(IOHIDDeviceRef device, int rid, CFIndex size)
{
    if (!size || !rid) {
        return raiseError(kIOReturnBadArgument, tr("Invalid parameters."));
    }

    IOReturn ret;
    CFIndex length = size;
    quint8 *buffer = (quint8 *) malloc(length);
    memset(buffer, 0x00, length);

    ret = IOHIDDeviceGetReport(device, kIOHIDReportTypeFeature, rid, buffer, &length);
    if (ret != kIOReturnSuccess) {
        free(buffer);
        return raiseError(ret, tr("Unable to read HID device."));
    }
    if (length == 0) {
        free(buffer);
        qWarning("[HIDDEV] Unexpected data length of HID report 0x%02x.", rid);
        return kIOReturnIOError;
    }

#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] GetReport(%p): [ID:0x%04x LEN:%lld] pl=%s", device, rid, d.length(), qPrintable(d.toHex(' ')));
#endif

    // return kIOReturnNotAttached if not match to requested profile
    ret = hidParseResponse(rid, buffer, length);
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
            raiseError(ret, tr("Unable to read HID device."));
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
                ret = raiseError(buffer->value, tr("Got critical status"));
                break;
            }
            case ROCCAT_CONTROL_VALUE_STATUS_INVALID: {
                ret = raiseError(buffer->value, tr("Got invalid status"));
                break;
            }
            default: {
                ret = raiseError(buffer->value, tr("Got unknown error"));
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
    IOReturn ret;
    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }

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

    return hidWriteWithCB(device, hrt, rid, buffer, length);
}

inline int RTHidDevice::hidWriteWithCB(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length)
{
    const CFTimeInterval timeout = 4.0f;
    const QDeadlineTimer dt(timeout * 1000, Qt::PreciseTimer);
    auto checkComplete = [this]() -> bool {
        QMutexLocker lock(&m_waitMutex);
        return m_isCBComplete;
    };

    m_isCBComplete = false;
    IOReturn ret = IOHIDDeviceSetReportWithCallback( //
        device,
        hrt,
        rid,
        buffer,
        length,
        timeout,
        _reportCallback,
        this);
    if (ret != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to write HID message."));
    }
    while (!checkComplete() && !dt.hasExpired()) {
        qApp->processEvents();
    }
    if (dt.hasExpired()) {
        return raiseError(kIOReturnTimeout, tr("Timeout while waiting for HID device."));
    }

    return ret;
}

inline int RTHidDevice::hidWriteReport(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length)
{
#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] hidWriteReport(%p): [RID:0x%02lx LEN:%ld] pl=%s", device, rid, length, qPrintable(d.toHex(' ')));
#endif

    IOReturn ret = IOHIDDeviceSetReport(device, hrt, rid, buffer, length);
    if (ret != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to write HID raw message."));
    }

    QThread::msleep(250);
    return ret;
}
