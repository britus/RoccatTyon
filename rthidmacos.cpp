#include "rthidmacos.h"
#include <DriverKit/IOUserClient.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <dispatch/dispatch.h>
#include <QByteArray>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>

Q_DECLARE_OPAQUE_POINTER(IOHIDDeviceRef)

#ifdef QT_DEBUG
static inline void debugDevice(IOHIDDeviceRef device)
{
    // Function to print a property based on its key and type
    auto printProperty = [](CFStringRef key, CFTypeRef value) {
        QString keyStr = QString::fromCFString(key);
        if (!value) {
            qWarning("[HIDDEV] %s: null", qPrintable(keyStr));
            return;
        }
        CFTypeID typeID = CFGetTypeID(value);
        if (typeID == CFStringGetTypeID()) {
            qDebug("[HIDDEV] %s: %s", //
                   qPrintable(keyStr),
                   qPrintable(QString::fromCFString((CFStringRef) value)));
        } else if (typeID == CFNumberGetTypeID()) {
            long num;
            CFNumberGetValue((CFNumberRef) value, kCFNumberLongType, &num);
            qDebug("[HIDDEV] %s: %ld (0x%lx)", //
                   qPrintable(keyStr),
                   num,
                   num);
        } else if (typeID == CFBooleanGetTypeID()) {
            bool b = CFBooleanGetValue((CFBooleanRef) value);
            qDebug("[HIDDEV] %s: %s", //
                   qPrintable(keyStr),
                   (b ? "true" : "false"));
        } else {
            // For unsupported types, use CFCopyDescription
            CFStringRef desc = CFCopyDescription(value);
            qDebug("[HIDDEV] %s: %s", //
                   qPrintable(keyStr),
                   qPrintable(QString::fromCFString(desc)));
            CFRelease(desc);
        }
    };

    qDebug("[HIDDEV] Device found ----------------------------------------");
    qDebug("[HIDDEV] Device: %p", device);

    // List of common property keys to query
    CFStringRef keys[] = //
        {CFSTR(kIOHIDTransportKey),
         CFSTR(kIOHIDVendorIDKey),
         CFSTR(kIOHIDVendorIDSourceKey),
         CFSTR(kIOHIDProductIDKey),
         CFSTR(kIOHIDVersionNumberKey),
         CFSTR(kIOHIDManufacturerKey),
         CFSTR(kIOHIDProductKey),
         CFSTR(kIOHIDSerialNumberKey),
         CFSTR(kIOHIDCountryCodeKey),
         CFSTR(kIOHIDLocationIDKey),
         CFSTR(kIOHIDDeviceUsageKey),
         CFSTR(kIOHIDPrimaryUsageKey),
         CFSTR(kIOHIDPrimaryUsagePageKey)};

    CFTypeRef value;
    size_t numKeys = sizeof(keys) / sizeof(keys[0]);
    for (size_t j = 0; j < numKeys; j++) {
        value = IOHIDDeviceGetProperty(device, keys[j]);
        printProperty(keys[j], value);
    }
}

static inline void debugReport(const char *where, IOHIDDeviceRef device, quint32 rid, const quint8 *buffer, CFIndex length)
{
    const QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] %s(%p): [RID:0x%02x LEN:%ld] pl=%s", where, device, rid, length, qPrintable(d.toHex(' ')));
}
#endif

RTHidMacOS::RTHidMacOS(QObject *parent)
    : RTAbstractDevice(parent)
    , m_manager(nullptr)
    , m_ctrlDevice(nullptr)
    , m_miscDevice(nullptr)
    , m_handlers()
    , m_waitMutex()
    , m_isCBComplete(false)
    , m_inputBuffer()
{
    qRegisterMetaType<IOHIDDeviceRef>();
}

RTHidMacOS::~RTHidMacOS()
{
    releaseDevices();
    releaseManager();
}

void RTHidMacOS::_deviceAttached(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
    if (!device || !context) {
        qCritical("[HIDDEV] _deviceAttached: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    debugDevice(device);
#endif

    RTHidMacOS *ctx;
    ctx = static_cast<RTHidMacOS *>(context);
    ctx->doDeviceFound(device);
}

/* -------------------------------------------------------
ROCCAT Tyon Mouse HID interfaces
    PrimaryUsage: 4 (0x4)           Mouse MCU interface
    PrimaryUsagePage: 1 (0x1)

    PrimaryUsage: 0 (0x0)           X-Cellerator input
    PrimaryUsagePage: 10 (0xa)

    PrimaryUsage: 6 (0x6)           unusable
    PrimaryUsagePage: 1 (0x1)

    PrimaryUsage: 2 (0x2)           unusable
    PrimaryUsagePage: 1 (0x1)
---------------------------------------------------------- */

static const uint kHIDUsageMouse = 0x04;
static const uint kHIDPageMouse = 0x01;

static const uint kHIDUsageMisc = 0x00;
static const uint kHIDPageMisc = 0x0a;

void RTHidMacOS::doDeviceFound(IOHIDDeviceRef device)
{
    THidDeviceInfo info = {};
    hidDeviceProperties(device, &info);

    THidDeviceType type;
    if ((info.primaryUsage == kHIDUsageMisc) && (info.primaryUsagePage == kHIDPageMisc)) {
        // Register device input callback to get special report for X-Celerator calibration
        IOHIDDeviceRegisterInputReportCallback(device, m_inputBuffer, m_inputLength, _deviceInput, this);
        type = THidDeviceType::HidMouseInput;
        m_miscDevice = device;
    } else if ((info.primaryUsage == kHIDUsageMouse) && (info.primaryUsagePage == kHIDPageMouse)) {
        type = THidDeviceType::HidMouseControl;
        m_ctrlDevice = device;
    } else {
        type = THidDeviceType::HidUnknown;
    }

    if (type != THidDeviceType::HidUnknown) {
        if (openDevice(type)) {
            emit deviceFound(type);
        }
    }
}

void RTHidMacOS::_deviceRemoved(void *context, IOReturn, void *, IOHIDDeviceRef device)
{
    if (!device || !context) {
        qCritical("[HIDDEV] _deviceRemoved: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    qDebug("[HIDDEV] _deviceRemoved: device '%p' removed", device);
#endif

    RTHidMacOS *ctx;
    ctx = static_cast<RTHidMacOS *>(context);
    ctx->doDeviceRemoved(device);
}

void RTHidMacOS::doDeviceRemoved(IOHIDDeviceRef device)
{
    if (device == m_ctrlDevice) {
        m_ctrlDevice = nullptr;
    }
    if (device == m_miscDevice) {
        m_miscDevice = nullptr;
    }

    if (m_ctrlDevice == nullptr && m_miscDevice == nullptr) {
        emit deviceRemoved();
    }
}

// Callback for IOHIDDeviceRegisterInputReportCallback
void RTHidMacOS::_deviceInput(void *context, IOReturn, void *device, IOHIDReportType, uint32_t rid, uint8_t *report, CFIndex length)
{
    if (!device || !context || !report) {
        qCritical("[HIDDEV] _deviceInput: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    debugReport("_deviceInput", (IOHIDDeviceRef) device, rid, (const quint8 *) report, length);
#endif

    RTHidMacOS *ctx;
    ctx = static_cast<RTHidMacOS *>(context);
    ctx->doDeviceInput(rid, length, report);
}

void RTHidMacOS::doDeviceInput(quint32 rid, qsizetype length, quint8 *report)
{
    QByteArray data;
    for (quint32 i = 0; i < length; i++) {
        data.append(report[i]);
    }
    emit inputReady(rid, data);
}

// Callback for IOHIDDeviceSetReportWithCallback
void RTHidMacOS::_reportSent(void *context, IOReturn result, void *device, IOHIDReportType, uint32_t rid, uint8_t *report, CFIndex length)
{
    if (!context || !device || !report) {
        qCritical("[HIDDEV] _reportSent: NULL pointer in required parameters!");
        return;
    }

#ifdef QT_DEBUG
    debugReport("_reportSent", (IOHIDDeviceRef) device, rid, (const quint8 *) report, length);
#endif

    RTHidMacOS *ctx;
    ctx = static_cast<RTHidMacOS *>(context);
    ctx->doReportSent(result, rid, length, report);
}

void RTHidMacOS::doReportSent(IOReturn, quint32, qsizetype, quint8 *)
{
    QMutexLocker lock(&m_waitMutex);
    m_isCBComplete = true;
}

bool RTHidMacOS::lookupDevices(quint32 vendorId, QList<quint32> productIds)
{
    IOReturn result;

    emit lookupStarted();

    // cleanup last findings
    releaseDevices();
    releaseManager();

    // start from top
    m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!m_manager) {
        raiseError(kIOReturnIOError, tr("Failed to create HID manager."));
        return false;
    }

    // Set up matching dictionary for Roccat Tyon devices
    CFMutableArrayRef filter = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

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
    foreach (auto productId, productIds) {
        if (!(dict = createMatching(vendorId, productId))) {
            CFRelease(m_manager);
            m_manager = nullptr;
            return false;
        }
        CFArrayAppendValue(filter, dict);
        CFRelease(dict);
    }

    IOHIDManagerSetDeviceMatchingMultiple(m_manager, filter);
    CFRelease(filter);

    // Register for device attached and removed callbacks and schedule in runloop
    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, _deviceAttached, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, _deviceRemoved, this);
    IOHIDManagerScheduleWithRunLoop(m_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // Open HID manager -536870174
    // kernel[0:1b4e77] (IOHIDFamily) IOHIDLibUserClient:0x0
    // [RoccatTyon] Entitlements 0 privilegedClient : No
    result = IOHIDManagerOpen(m_manager, kIOHIDOptionsTypeSeizeDevice);
    if (result != kIOReturnSuccess          //
        && result != kIOReturnNotPrivileged //
        && result != kIOReturnNotPermitted) //
    {
        raiseError(result, tr("Failed to open HID manager."));
        CFRelease(m_manager);
        m_manager = nullptr;
        return false;
    }

    // hold loop in separated thread for monitoring
    std::thread([]() { CFRunLoopRun(); }).detach();

    return true;
}

/**
     * @brief Register HID report handlers
     * @param handlers A map of reportId / handler function
     */
void RTHidMacOS::registerHandlers(const TReportHandlers &handlers)
{
    m_handlers.clear();
    m_handlers = handlers;
}

bool RTHidMacOS::openDevice(THidDeviceType type)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }

    IOReturn ret = IOHIDDeviceOpen(device, kIOHIDOptionsTypeSeizeDevice);
    if (ret != kIOReturnSuccess) {
        raiseError(kIOReturnNoDevice, "Unable to open device.");
        return false;
    }

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Device(%p) opened", device);
#endif

    return true;
}

bool RTHidMacOS::closeDevice(THidDeviceType type)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }

    IOReturn ret = IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
    return ret == kIOReturnSuccess;
}

bool RTHidMacOS::hasDevice() const
{
    return (m_ctrlDevice != nullptr) && (m_miscDevice != nullptr);
}

bool RTHidMacOS::readHidMessage(THidDeviceType type, quint32 reportId, qsizetype length)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }
    return hidReadAsync(device, reportId, length) == kIOReturnSuccess;
}

bool RTHidMacOS::readHidMessage(THidDeviceType type, quint32 reportId, quint8 *buffer, qsizetype length)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }
    return hidReadReport(device, reportId, buffer, length) == kIOReturnSuccess;
}

bool RTHidMacOS::writeHidMessage(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }
    return hidWriteReport(device, reportId, buffer, length) == kIOReturnSuccess;
}

bool RTHidMacOS::writeHidAsync(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length)
{
    IOHIDDeviceRef device;
    if (!(device = toDevice(type))) {
        raiseError(kIOReturnNoDevice, "HID device not connected.");
        return false;
    }
    return hidWriteAsync(device, reportId, buffer, length) == kIOReturnSuccess;
}

inline int RTHidMacOS::raiseError(int error, const QString &message)
{
    qCritical("[HIDDEV] Error 0x%08x: %s", error, qPrintable(message));
    emit errorOccured(error, message);
    return error;
}

inline void RTHidMacOS::releaseManager()
{
    if (m_manager) {
        IOHIDManagerUnscheduleFromRunLoop(m_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
        CFRelease(m_manager);
        m_manager = nullptr;
    }
}

inline void RTHidMacOS::releaseDevices()
{
    if (m_ctrlDevice != nullptr) {
        IOHIDDeviceClose(m_ctrlDevice, kIOHIDOptionsTypeSeizeDevice);
        m_ctrlDevice = nullptr;
    }
    if (m_miscDevice != nullptr) {
        IOHIDDeviceClose(m_miscDevice, kIOHIDOptionsTypeSeizeDevice);
        m_miscDevice = nullptr;
    }
}

inline IOHIDDeviceRef RTHidMacOS::toDevice(THidDeviceType type)
{
    switch (type) {
        case HidMouseControl: {
            return m_ctrlDevice;
        }
        case HidMouseInput: {
            return m_miscDevice;
        }
        default: {
            break;
        }
    }
    return nullptr;
}

inline void RTHidMacOS::hidDeviceProperties(IOHIDDeviceRef device, THidDeviceInfo *info) const
{
    if (!device || !info) {
        return;
    }

    // Function to print a property based on its key and type
    auto printProperty = [](const CFStringRef key, CFTypeRef value, THidDeviceInfo *info) {
        QString keyStr = QString::fromCFString(key);
        if (!value) {
            //qWarning("[HIDDEV] HID device property '%s' null value", qPrintable(keyStr));
            return;
        }
        CFTypeID typeID = CFGetTypeID(value);
        if (typeID == CFStringGetTypeID()) {
            if (keyStr == kIOHIDTransportKey) {
                info->transport = QString::fromCFString((CFStringRef) value);
            } else if (keyStr == kIOHIDManufacturerKey) {
                info->manufacturer = QString::fromCFString((CFStringRef) value);
            } else if (keyStr == kIOHIDProductKey) {
                info->product = QString::fromCFString((CFStringRef) value);
            } else if (keyStr == kIOHIDSerialNumberKey) {
                info->serialNumber = QString::fromCFString((CFStringRef) value);
            } else if (keyStr == kIOHIDDeviceUsageKey) {
                info->deviceUsage = QString::fromCFString((CFStringRef) value);
            }
        } else if (typeID == CFNumberGetTypeID()) {
            CFOptionFlags num;
            CFNumberGetValue((CFNumberRef) value, kCFNumberLongType, &num);
            if (keyStr == kIOHIDVendorIDKey) {
                info->vendorId = static_cast<uint>(num);
            } else if (keyStr == kIOHIDVendorIDSourceKey) {
                info->vendorIdSource = static_cast<uint>(num);
            } else if (keyStr == kIOHIDProductIDKey) {
                info->productId = static_cast<uint>(num);
            } else if (keyStr == kIOHIDVersionNumberKey) {
                info->versionNumber = static_cast<uint>(num);
            } else if (keyStr == kIOHIDCountryCodeKey) {
                info->countryCode = static_cast<uint>(num);
            } else if (keyStr == kIOHIDLocationIDKey) {
                info->locationId = static_cast<uint>(num);
            } else if (keyStr == kIOHIDPrimaryUsageKey) {
                info->primaryUsage = static_cast<uint>(num);
            } else if (keyStr == kIOHIDPrimaryUsagePageKey) {
                info->primaryUsagePage = static_cast<uint>(num);
            }
        } else if (typeID == CFBooleanGetTypeID()) {
            //bool b = CFBooleanGetValue((CFBooleanRef) value);
        } else {
            // For unhandled types, use CFCopyDescription
            CFStringRef desc = CFCopyDescription(value);
            qWarning("[HIDDEV] Unhandled HID device property %s: %s", //
                     qPrintable(keyStr),
                     qPrintable(QString::fromCFString(desc)));
            CFRelease(desc);
        }
    };

    // List of common property keys to query
    const CFStringRef keys[13] = //
        {CFSTR(kIOHIDTransportKey),
         CFSTR(kIOHIDVendorIDKey),
         CFSTR(kIOHIDVendorIDSourceKey),
         CFSTR(kIOHIDProductIDKey),
         CFSTR(kIOHIDVersionNumberKey),
         CFSTR(kIOHIDManufacturerKey),
         CFSTR(kIOHIDProductKey),
         CFSTR(kIOHIDSerialNumberKey),
         CFSTR(kIOHIDCountryCodeKey),
         CFSTR(kIOHIDLocationIDKey),
         CFSTR(kIOHIDDeviceUsageKey),
         CFSTR(kIOHIDPrimaryUsageKey),
         CFSTR(kIOHIDPrimaryUsagePageKey)};

    CFTypeRef value;
    for (size_t j = 0; j < 13; j++) {
        value = IOHIDDeviceGetProperty(device, keys[j]);
        printProperty(keys[j], value, info);
    }
}

inline int RTHidMacOS::hidReadAsync(IOHIDDeviceRef device, CFIndex rid, CFIndex length)
{
    if (!length || !rid) {
        return raiseError(kIOReturnBadArgument, tr("Invalid parameters."));
    }

    IOReturn ret = kIOReturnSuccess;
    quint8 *buffer = (quint8 *) malloc(length);
    memset(buffer, 0, length);

    if ((ret = hidReadReport(device, rid, buffer, length)) != kIOReturnSuccess) {
        free(buffer);
        return ret;
    }

    if (!m_handlers.contains(rid)) {
        qWarning("[HIDEV] Unhandled HID report RID=0x%02lx", rid);
        QByteArray data;
        for (quint32 i = 0; i < length; i++) {
            data.append(buffer[i]);
        }
        emit inputReady(rid, data);
        goto func_exit;
    }

    if (!m_handlers[rid](buffer, length)) {
        qWarning("[HIDEV] HID report handler return with failue. RID=0x%02lx", rid);
        goto func_exit;
    }

func_exit:
    free(buffer);
    return ret;
}

inline int RTHidMacOS::hidReadReport(IOHIDDeviceRef device, CFIndex rid, quint8 *buffer, CFIndex length)
{
    const IOHIDReportType hrt = kIOHIDReportTypeFeature;

    IOReturn ret = IOHIDDeviceGetReport(device, hrt, rid, buffer, &length);
    if (ret != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to read HID device."));
    }

    if (length == 0) {
        qWarning("[HIDDEV] Unexpected data length of HID report 0x%02lx.", rid);
        return kIOReturnIOError;
    }

#ifdef QT_DEBUG
    debugReport("hidReadReport", device, rid, buffer, length);
#endif

    return ret;
}

inline int RTHidMacOS::hidWriteReport(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length)
{
    const IOHIDReportType hrt = kIOHIDReportTypeFeature;

#ifdef QT_DEBUG
    debugReport("hidWriteReport", device, rid, buffer, length);
#endif

    IOReturn ret = IOHIDDeviceSetReport(device, hrt, rid, buffer, length);
    if (ret != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to write HID raw message."));
    }

    // make sure device MCU is ready for next
    QThread::msleep(150);
    return ret;
}

inline int RTHidMacOS::hidWriteAsync(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length)
{
    const IOHIDReportType hrt = kIOHIDReportTypeFeature;
    const CFTimeInterval timeout = 4.0f; // seconds timeout
    const QDeadlineTimer dt(timeout * 1000, Qt::PreciseTimer);

    auto checkComplete = [this]() -> bool {
        QMutexLocker lock(&m_waitMutex);
        return m_isCBComplete;
    };

#ifdef QT_DEBUG
    debugReport("hidWriteAsync", device, rid, buffer, length);
#endif

    m_isCBComplete = false;

    IOReturn ret = IOHIDDeviceSetReportWithCallback(device, hrt, rid, buffer, length, timeout, _reportSent, this);
    if (ret != kIOReturnSuccess) {
        return raiseError(ret, tr("Unable to write HID message."));
    }

    while (!checkComplete() && !dt.hasExpired()) {
        qApp->processEvents();
    }

    if (dt.hasExpired()) {
        return raiseError(kIOReturnTimeout, tr("Timeout while waiting for HID device."));
    }

    // make sure device MCU is ready for next
    QThread::msleep(150);
    return ret;
}
