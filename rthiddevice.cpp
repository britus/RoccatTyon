#include "rthiddevice.h"
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

//#undef QT_DEBUG

#ifdef QT_DEBUG
#include "rthiddevicedbg.hpp"
#endif

Q_DECLARE_OPAQUE_POINTER(IOHIDDeviceRef)

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
    for (quint8 i = 0; i < TYON_PROFILE_NUM; i++) {
        TProfile profile;
        profile.index = i;
        profile.name = tr("Default_%1").arg(profile.index);
        m_profiles[i] = profile;
    }
}

RTHidDevice::~RTHidDevice()
{
    hid_exit();
    releaseManager();
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

    // Register for device attached and removed callbacks
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

    //#ifndef QT_DEBUG
    // Run event loop in separate thread
    std::thread([]() { CFRunLoopRun(); }).detach();
    //#endif

    emit lookupStarted();
}

bool RTHidDevice::resetProfiles()
{
    TyonInfo info = {};
    const quint8 *buffer = (const quint8 *) &info;
    info.report_id = TYON_REPORT_ID_INFO;
    info.size = sizeof(TyonInfo);
    info.function = TYON_INFO_FUNCTION_RESET;

    m_profiles.clear();
    m_profile = {};
    m_info = {};

    // Run event loop in separate thread
    IOReturn ret;
    foreach (IOHIDDeviceRef device, m_devices) {
        if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
            return ret;
        }
        if (hidSetReportRaw(device, buffer, info.size) == kIOReturnSuccess) {
            break;
        }
    }

    QThread::yieldCurrentThread();

    releaseManager();
    lookupDevice();

    return true;
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
                if ((ret = writeProfile(device, p)) != kIOReturnSuccess) {
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

#if 0
    QTimer::singleShot(200, this, [this, writeIndex, writeProfile]() {
        IOReturn ret;
        foreach (IOHIDDeviceRef device, m_devices) {
            if ((ret = writeIndex(device)) != kIOReturnSuccess) {
                return false;
            }
            foreach (const TProfile p, m_profiles) {
                if ((ret = writeProfile(device, p)) != kIOReturnSuccess) {
                    return false;
                }
            }
        }

        releaseManager();
        lookupDevice();
    });
#endif
    return true;
}

bool RTHidDevice::saveProfilesToFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::Truncate | QFile::WriteOnly)) {
        return false;
    }

    QDataStream s(&f);
    s << 0xbe250566;
    foreach (const TProfile p, m_profiles) {
        s << p.index;
        s << p.name;
        s << 0xba000000;
        s << p.buttons.profile_index;
        for (quint8 i = 0; i < TYON_PROFILE_BUTTON_NUM; i++) {
            s << p.buttons.buttons[i].type;
            s << p.buttons.buttons[i].key;
            s << p.buttons.buttons[i].modifier;
        }
        s << 0xbb000001;
        s << p.settings.advanced_sensitivity;
        s << p.settings.sensitivity_x;
        s << p.settings.sensitivity_y;
        s << p.settings.cpi_levels_enabled;
        s << p.settings.cpi_active;
        s << p.settings.talkfx_polling_rate;
        s << p.settings.lights_enabled;
        s << p.settings.color_flow;
        s << p.settings.light_effect;
        s << p.settings.effect_speed;
        s << 0xbb000002;
        for (quint8 i = 0; i < TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM; i++) {
            s << p.settings.cpi_levels[i];
        }
        s << 0xbb000003;
        for (quint8 i = 0; i < TYON_LIGHTS_NUM; i++) {
            s << p.settings.lights[i].index;
            s << p.settings.lights[i].red;
            s << p.settings.lights[i].blue;
            s << p.settings.lights[i].green;
            s << p.settings.lights[i].unused;
        }
        s << p.settings.checksum;
    }
    s << 0xbe660525;

    f.flush();
    f.close();
    return true;
}

bool RTHidDevice::loadProfilesFromFile(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        return false;
    }
    // TODO: read from encrypted file
    f.close();
    return true;
}

void RTHidDevice::assignButton( //
    TyonButtonIndex type,
    TyonButtonType func,
    quint8 key,
    quint8 mods)
{
    if ((qint8) type > TYON_PROFILE_BUTTON_NUM) {
        return;
    }
    if (!m_profiles.contains(profileIndex())) {
        return;
    }

    TProfile p = m_profiles[profileIndex()];
    RoccatButton *b = &p.buttons.buttons[type];
    if (b->type != func || b->modifier != mods || b->key != key) {
        b->type = func;
        b->modifier = mods;
        b->key = key;
        m_profiles[profileIndex()] = p;
        emit profileChanged(p);
    }
}

void RTHidDevice::setActiveProfile(quint8 profileIndex)
{
    if (m_profile.profile_index != profileIndex) {
        m_profile.profile_index = profileIndex;
        emit profileIndexChanged(profileIndex);
        if (!m_profiles.contains(profileIndex)) {
            m_profiles[profileIndex].index = 0;
            m_profiles[profileIndex].name = tr("Profile %1").arg(0);
            m_profiles[profileIndex].buttons = {};
            m_profiles[profileIndex].settings = {};
        }
        emit profileChanged(m_profiles[profileIndex]);
    }
}

void RTHidDevice::setProfileName(const QString &name, quint8 profileIndex)
{
    if (m_profiles.contains(profileIndex)) {
        TProfile p = m_profiles[profileIndex];
        if (p.name != name) {
            p.name = name;
            m_profiles[profileIndex] = p;
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
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setDpiLevel(quint8 index, quint8 value)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.cpi_levels[index] != value) {
            p.settings.cpi_levels[index] = value;
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
    if ((ret = hidCheckWrite(device)) != kIOReturnSuccess) {
        return ret;
    }
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

    if (pix > TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index parameter.");
        return EINVAL;
    }

    if (bix > TYON_PROFILE_BUTTON_NUM) {
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
    IOReturn ret;

    if (pix > TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    /* roccat_select:
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: TYON_CONTROL_DATA_INDEX_NONE | profile_index,
     *  request: TYON_CONTROL_REQUEST_PROFILE_SETTINGS,
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 DIX = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    const quint8 REQ = TYON_CONTROL_REQUEST_PROFILE_SETTINGS;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile settings. DIX=0x%02x PIX=%d REQ=0x%02x", DIX, pix, REQ);
#endif

    if ((ret = hidSetRoccatControl(device, HEP, RID, DIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=0x%08x", pix, ret);
        return ret;
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::selectProfileButtons(IOHIDDeviceRef device, uint pix)
{
    IOReturn ret;

    if (pix > TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    /* roccat_select:
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: TYON_CONTROL_DATA_INDEX_NONE | profile_index,
     *  request: TYON_CONTROL_REQUEST_PROFILE_BUTTONS,
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 DIX = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    const quint8 REQ = TYON_CONTROL_REQUEST_PROFILE_BUTTONS;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select profile buttons. DIX=0x%02x PIX=%d REQ=0x%02x", DIX, pix, REQ);
#endif

    if ((ret = hidSetRoccatControl(device, HEP, RID, DIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=0x%08x", pix, ret);
        return ret;
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix)
{
    IOReturn ret;

    /* roccat_select(
     *  device: device,
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: (TYON_CONTROL_DATA_INDEX_MACRO_1 or _2) | profile_index,
     *  request: bix,
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 DIX = (dix | pix);
    const quint8 REQ = bix;

#ifdef QT_DEBUG
    qDebug("[HIDDEV] Select macro. DIX=0x%02x PIX=%d REQ=0x%02x", DIX, pix, REQ);
#endif

    if ((ret = hidSetRoccatControl(device, HEP, RID, DIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select macro %d. ret=0x%08x", REQ, ret);
        return ret;
    }

    return kIOReturnSuccess;
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

inline int RTHidDevice::hidSetRoccatControl(IOHIDDeviceRef device, uint ep, uint rid, uint pix, uint req)
{
    /* ROCCAT control message */
    RoccatControl control;
    control.report_id = rid;
    control.request = req;
    control.value = pix;

    m_isCBComplete = false;
    const IOHIDReportType hrt = toMacOSReportType(ep);
    const CFIndex length = sizeof(RoccatControl);
    const uint8_t *buffer = (const uint8_t *) &control;
    IOReturn ret = kIOReturnSuccess;

#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] SetRoccatControl(%p): [RID:0x%04x LEN:%lld] pl=%s", device, control.report_id, d.length(), qPrintable(d.toHex(' ')));
#endif

#if 1
    const CFTimeInterval timeout = 2.0f;
    ret = IOHIDDeviceSetReportWithCallback( //
        device,
        hrt,
        rid,
        buffer,
        length,
        timeout,         // Timeout in Sekunden
        _reportCallback, // Callback-Funktion
        this             // context
    );

    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to send report 0x%02x with request 0x%04x", rid, req);
        return ret;
    }

    auto checkComplete = [this]() -> bool {
        QMutexLocker lock(&m_mutex);
        return m_isCBComplete;
    };

    QDeadlineTimer dt(500, Qt::PreciseTimer);
    do {
        if (checkComplete()) {
            break;
        }
        qApp->processEvents();
    } while (!dt.hasExpired());

    if (dt.hasExpired()) {
        qCritical("[HIDDEV] Timeout while waiting for HID device %p.", device);
        return EIO;
    }
#else
    ret = IOHIDDeviceSetReport(device, hrt, rid, buffer, sizeof(RoccatControl));
#endif

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
        qDebug("[HIDDEV] hidCheckWrite(%p): [RID:0x%04x LEN:%lld] pl=%s", device, rid, d.length(), qPrintable(d.toHex(' ')));
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

inline int RTHidDevice::hidSetReportRaw(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length)
{
    IOReturn ret;

    const IOHIDReportType hrt = toMacOSReportType(TYON_INTERFACE_MOUSE);
    const quint8 rid = buffer[0]; // required!

#ifdef QT_DEBUG
    QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] hidSetReportRaw(%p): [RID:0x%04x LEN:%lld] pl=%s", device, rid, d.length(), qPrintable(d.toHex(' ')));
#endif

    m_isCBComplete = false;
    ret = IOHIDDeviceSetReport(device, hrt, rid, buffer, length);
    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to write HID raw message. ret=0x%08x", ret);
        return ret;
    }

    QThread::msleep(250);
    return kIOReturnSuccess;
}
