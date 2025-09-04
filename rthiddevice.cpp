#include "rthiddevice.h"
#include "rttypes.h"
#include <IOKit/hid/IOHIDManager.h>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QMutexLocker>
#include <QThread>

static int ROCCAT_VENDOR_ID = 0x1e7d;
static int ROCCAT_TYON_RID = 0x2e4a;

Q_DECLARE_OPAQUE_POINTER(IOHIDDeviceRef)

RTHidDevice::RTHidDevice(QObject *parent)
    : QObject{parent}
    , m_manager(nullptr)
    , m_devices()
    , m_info()
    , m_profile()
    , m_profiles()
    , m_mutex()
    , m_isCBComplete(false)
{
    qRegisterMetaType<IOHIDDeviceRef>();

    connect( //
        this,
        &RTHidDevice::deviceFoundCallback,
        this,
        &RTHidDevice::onDeviceFound,
        Qt::QueuedConnection);
    connect( //
        this,
        &RTHidDevice::reportCallback,
        this,
        &RTHidDevice::onSetReportCallback,
        Qt::QueuedConnection);
}

RTHidDevice::~RTHidDevice()
{
    releaseManager();
}

/* Called by Mac OSX multiple times for same single device */
static inline void deviceMatched(void *context, IOReturn result, void *, IOHIDDeviceRef device)
{
    if (result != 0) {
        qCritical("[HIDDEV] Device matcher IOResult not zero: %d", result);
        return;
    }

    IOReturn ret = IOHIDDeviceOpen(device, kIOHIDOptionsTypeSeizeDevice);
    if (ret != kIOReturnSuccess) {
        return;
    }

    qDebug("[HIDDEV] Device %p found. -----------------------------", device);

    RTHidDevice *ctx;
    if ((ctx = dynamic_cast<RTHidDevice *>((QObject *) context))) {
        //emit ctx->deviceFoundCallback(result, device);
        ctx->onDeviceFound(result, device);
    }
}

int RTHidDevice::lookupDevice()
{
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        qCritical() << "[HIDDEV] Unable to create IOHIDManager instance.";
        return EINVAL;
    }

    // Device-Matching: Only ROCCAT Tyon
    CFMutableDictionaryRef matchingDict = CFDictionaryCreateMutable( //
        kCFAllocatorDefault,
        0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFNumberRef vendorId = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &ROCCAT_VENDOR_ID);
    CFNumberRef productId = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &ROCCAT_TYON_RID);

    CFDictionarySetValue(matchingDict, CFSTR(kIOHIDVendorIDKey), vendorId);
    CFDictionarySetValue(matchingDict, CFSTR(kIOHIDProductIDKey), productId);

    IOHIDManagerSetDeviceMatching(manager, matchingDict);
    CFRelease(matchingDict);
    CFRelease(vendorId);
    CFRelease(productId);

    IOHIDManagerRegisterDeviceMatchingCallback(manager, deviceMatched, this);
    IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);

    return kIOReturnSuccess;
}

bool RTHidDevice::resetProfiles()
{
    foreach (IOHIDDeviceRef device, m_devices) {
        if (deviceReset(device) == kIOReturnSuccess) {
            return true;
        }
    }
    return false;
}

bool RTHidDevice::saveProfilesToDevice()
{
    return false;
}

void RTHidDevice::setActiveProfile(quint8 profileIndex)
{
    if (m_profile.profile_index != profileIndex) {
        m_profile.profile_index = profileIndex;
        emit profileIndexChanged(profileIndex);
        if (m_profiles.contains(profileIndex)) {
            emit profileChanged(m_profiles[profileIndex]);
        }
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

void RTHidDevice::setLightColorWheel(quint8 red, quint8 green, quint8 blue)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.lights[0].red != red        //
            || p.settings.lights[0].green != green //
            || p.settings.lights[0].blue != blue) {
            p.settings.lights[0].red = red;
            p.settings.lights[0].green = green;
            p.settings.lights[0].blue = blue;
            m_profiles[profileIndex()] = p;
            emit profileChanged(p);
        }
    }
}

void RTHidDevice::setLightColorBottom(quint8 red, quint8 green, quint8 blue)
{
    if (m_profiles.contains(profileIndex())) {
        TProfile p = m_profiles[profileIndex()];
        if (p.settings.lights[0].red != red        //
            || p.settings.lights[0].green != green //
            || p.settings.lights[0].blue != blue) {
            p.settings.lights[1].red = red;
            p.settings.lights[1].green = green;
            p.settings.lights[1].blue = blue;
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

void RTHidDevice::onDeviceFound(IOReturn /*status*/, IOHIDDeviceRef device)
{
    IOReturn ret;

    /* skip if already read */
    if (m_devices.contains(device)) {
        return;
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
    return;

error_exit:
    IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
}

inline int RTHidDevice::readProfile(IOHIDDeviceRef device, quint8 pix)
{
    IOReturn ret;

    /* select profile settings store */
    if ((ret = selectProfileSettings(device, pix)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=%d", pix, ret);
        return ret;
    }

    /* read profile settings */
    if ((ret = readProfileSettings(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read profile settings. ret=%d", ret);
        return ret;
    }

    /* select profile buttons store */
    if ((ret = selectProfileButtons(device, pix)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=%d", pix, ret);
        return ret;
    }

    /* read profile buttons */
    if ((ret = readProfileButtons(device)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read profile buttons. ret=%d", ret);
        return ret;
    }

    /* read all button slots including combined with EasyShift */
    for (quint8 bix = 0; bix < TYON_PROFILE_BUTTON_NUM; bix++) {
        if ((ret = readButtonMacro(device, pix, bix)) != kIOReturnSuccess) {
            if (ret != ENODATA) { // optional macros
                qCritical("[HIDDEV] Unable to read profile macro #%d. ret=%d", bix, ret);
                return ret;
            }
        }
    }

    return kIOReturnSuccess;
}

inline void RTHidDevice::releaseManager()
{
    IOHIDDeviceRef dev;
    while (!m_devices.isEmpty()) {
        if ((dev = m_devices.takeFirst())) {
            try {
                IOHIDDeviceClose(dev, kIOHIDOptionsTypeSeizeDevice);
            } catch (...) {
            }
        }
    }
    m_devices.clear();

    if (m_manager) {
        CFRelease(m_manager);
    }
}

inline int RTHidDevice::readDeviceSpecial(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_SPECIAL, 256);
}

inline int RTHidDevice::readDeviceControl(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_CONTROL, 3);
}

inline int RTHidDevice::readActiveProfile(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_PROFILE, 3);
}

inline int RTHidDevice::readProfileSettings(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_PROFILE_SETTINGS, 30);
}

inline int RTHidDevice::readProfileButtons(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_PROFILE_BUTTONS, sizeof(TyonProfileButtons));
}

inline int RTHidDevice::readDeviceInfo(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_INFO, sizeof(TyonInfo));
}

inline int RTHidDevice::readDevice0A(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_A, 8);
}

inline int RTHidDevice::readDeviceSensor(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_SENSOR, 4);
}

inline int RTHidDevice::readDeviceState(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_DEVICE_STATE, 3);
}

inline int RTHidDevice::readDeviceControlUnit(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_CONTROL_UNIT, 6);
}

inline int RTHidDevice::readDeviceTalk(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_TALK, 16);
}

inline int RTHidDevice::readDevice11(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_11, 16);
}

inline int RTHidDevice::readDevice1A(IOHIDDeviceRef device)
{
    return readHidReport(device, TYON_REPORT_ID_1A, 1029);
}

static inline void debugSettings(const RTHidDevice::TProfile profile, quint8 currentPix)
{
    const TyonProfileSettings *s = &profile.settings;

    qDebug("[HIDDEV] PROFILE: INDEX=%d NAME=%s", //
           profile.index,
           qPrintable(profile.name));

    qDebug("[HIDDEV] SETTINGS: SIZE=%d PIX=%d (ACT=%d) CHKS=%d", s->size, s->profile_index, currentPix, s->checksum);
    qDebug("[HIDDEV] SETTINGS: DPI_LEVs_EN=%d DPI_ACTIVE=%d SENS_X=%d SENS_Y=%d ADV_SENS=%d TFX_POLLRATE=%d", //
           s->cpi_levels_enabled,
           s->cpi_active,
           s->sensitivity_x,
           s->sensitivity_y,
           s->advanced_sensitivity,
           s->talkfx_polling_rate);

    for (qint8 i = 0; i < TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM; i++) {
        qDebug("[HIDDEV] SETTINGS: DPI-%d DPI-LEVEL=%d ((>>2)*200 = %d)", //
               i,
               s->cpi_levels[i],
               (s->cpi_levels[i] >> 2) * 200);
    }

    qDebug("[HIDDEV] SETTINGS: LIGHT_EN=%d LIGHT_EFF=0x%02x CLR_FLOW=0x%02x SPEED=%d", //
           s->lights_enabled,
           s->light_effect,
           s->color_flow,
           s->effect_speed);

    for (qint8 i = 0; i < TYON_LIGHTS_NUM; i++) {
        qDebug("[HIDDEV] SETTINGS: LIGHT-%d RED=%d GREEN=%d BLUE=%d IDX=%d", //
               i,
               s->lights[i].red,
               s->lights[i].green,
               s->lights[i].blue,
               s->lights[i].index);
    }
}

static inline void debugButtons(const RTHidDevice::TProfile profile, quint8 currentPix)
{
    const TyonProfileButtons *b = &profile.buttons;

    qDebug("[HIDDEV] BUTTONS: SIZE=%d PIX=%d (ACT=%d)", b->size, b->profile_index, currentPix);

    /* physical buttons and with EasyShift combined
     * 32 buttons (16x2) */
    const quint8 bixStart = TYON_BUTTON_INDEX_LEFT;
    const quint8 bixCount = TYON_PROFILE_BUTTON_NUM;

    for (quint8 index = bixStart; index < bixCount; index++) {
        qDebug("[HIDDEV] BUTTONS: #%02d B_TYPE=0x%04x B_KEY=0x%02x '%c' B_MOD=0x%04x", //
               index,
               b->buttons[index].type, /* -> button function */
               b->buttons[index].key,
               b->buttons[index].key >= 0x20 && b->buttons[index].key < 0x7f ? b->buttons[index].key : ' ',
               b->buttons[index].modifier);
    }
}

static inline void debugDevInfo(TyonInfo *p)
{
    qDebug("[HIDDEV] DEVINFO: SIZE=%d DFU=%d FW=%d", //
           p->size,
           p->dfu_version,
           p->firmware_version);
    qDebug("[HIDDEV] DEVINFO: XC_MIN=%d XC_MID=%d XC_MAX=%d", //
           p->xcelerator_min,
           p->xcelerator_mid,
           p->xcelerator_max);
    qDebug("[HIDDEV] DEVINFO: FUNCTION=%d", p->function);
}

// Read ROCCAT Mouse report descriptor
inline int RTHidDevice::readHidReport(IOHIDDeviceRef device, const int reportId, const CFIndex size)
{
    if (!size || !reportId) {
        qCritical() << "[HIDDEV] Invalid parameters.";
        return EINVAL;
    }

    IOReturn ret;
    const int HID_SIZE = 128;
    CFIndex length = (size > HID_SIZE ? size : HID_SIZE);
    quint8 *buffer = (quint8 *) malloc(length);
    memset(buffer, 0x00, length);

    ret = IOHIDDeviceGetReport(device, kIOHIDReportTypeFeature, reportId, buffer, &length);
    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to read HID device.");
        free(buffer);
        return EIO;
    }
    if (buffer[0] != reportId) {
        qCritical("[HIDDEV] Invalid HID report data.");
        free(buffer);
        return EIO;
    }
    if (buffer[1] != size) {
        if (buffer[1] != 0) {
            qWarning("[HIDDEV] No data for report 0x%04x responsed, skip.", reportId);
        }
        free(buffer);
        return ENODATA;
    }

    // set descriptor length
    length = buffer[1];

    QByteArray payload;
    for (CFIndex i = 0; i < length; i++) {
        payload.append(buffer[i]);
    }
    free(buffer);

    QThread::yieldCurrentThread();

    qDebug("[HIDDEV] Payload(%p): [ID:0x%04x LEN:%lld] %s", //
           device,
           reportId,
           payload.length(),
           qPrintable(payload.toHex(' ')));

    auto toProfile = [this](quint8 pix) -> TProfile {
        TProfile profile = {};
        if (m_profiles.contains(pix)) {
            profile = m_profiles[pix];
        } else {
            profile.index = pix;
            profile.name = tr("Default_%1").arg(profile.index);
        }
        return profile;
    };

    TProfile profile = {};
    switch (reportId) {
        case TYON_REPORT_ID_INFO: {
            TyonInfo *p = (TyonInfo *) payload.constData();
            memcpy(&m_info, p, sizeof(TyonInfo));
            debugDevInfo(&m_info);
            emit deviceInfo(m_info);
            break;
        }
        case TYON_REPORT_ID_PROFILE: {
            TyonProfile *p = (TyonProfile *) payload.constData();
            qDebug("[HIDDEV] PROFILE: ACTIVE_PROFILE=%d", p->profile_index);
            memcpy(&m_profile, p, sizeof(TyonProfile));
            emit profileIndexChanged(p->profile_index);
            break;
        }
        case TYON_REPORT_ID_PROFILE_SETTINGS: {
            TyonProfileSettings *p = (TyonProfileSettings *) payload.constData();
            profile = toProfile(p->profile_index);
            memcpy(&profile.settings, p, sizeof(TyonProfileSettings));
            m_profiles[profile.index] = profile;
            debugSettings(profile, m_profile.profile_index);
            emit profileChanged(profile);
            break;
        }
        case TYON_REPORT_ID_PROFILE_BUTTONS: {
            TyonProfileButtons *p = (TyonProfileButtons *) payload.constData();
            profile = toProfile(p->profile_index);
            memcpy(&profile.buttons, p, sizeof(TyonProfileButtons));
            m_profiles[profile.index] = profile;
            debugButtons(profile, m_profile.profile_index);
            emit profileChanged(profile);
            break;
        }
        case TYON_REPORT_ID_MACRO: {
            TyonMacro *p = (TyonMacro *) payload.constData();
            qDebug("[HIDDEV] MACRO: #%02d group=%s name=%s", //
                   p->button_index,
                   p->macroset_name,
                   p->macro_name);
        }
        case TYON_REPORT_ID_DEVICE_STATE: {
            TyonDeviceState *p = (TyonDeviceState *) payload.constData();
            qDebug("[HIDDEV] STATE: state=%d", p->state);
        }
    }

    return kIOReturnSuccess;
}

// Callback for IOHIDDeviceSetReportWithCallback
static inline void writeReportCallback( //
    void *context,
    IOReturn result,
    void *,
    IOHIDReportType type,
    uint32_t reportID,
    uint8_t *report,
    CFIndex reportLength)
{
#if 1
    qDebug("[HIDDEV] (WRCB) IOHIDDeviceSetReportWithCallback result=%d", result);
    qDebug("[HIDDEV] (WRCB) RT=%d RID=%d SIZE=%ld CTX=%p", type, reportID, reportLength, context);
#endif

    RTHidDevice *ctx;
    if ((ctx = dynamic_cast<RTHidDevice *>((QObject *) context))) {
        const QByteArray data((char *) report, reportLength);
        //emit ctx->reportCallback(result, reportID, data.length(), data);
        ctx->onSetReportCallback(result, reportID, data.length(), data);
    }
}

void RTHidDevice::onSetReportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data)
{
    QMutexLocker lock(&m_mutex);

    if (length > 0) {
        qDebug("[HIDDEV] status=%d RID=%d SIZE=%ld", status, rid, length);
        qDebug("[HIDDEV] data=%s", qPrintable(data.toHex(' ')));
    }

    m_isCBComplete = true;
}

// Send report to ROCCAT mouse
inline int RTHidDevice::writeHidReport(IOHIDDeviceRef device, uint ep, uint rid, uint pix, uint req)
{
    IOReturn ret;
    RoccatControl control;
    control.report_id = rid;
    control.request = req;
    control.value = pix;

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

    m_isCBComplete = false;

    const CFTimeInterval timeout = 2.0f;
    const CFIndex payloadSize = sizeof(RoccatControl);
    const uint8_t *payload = (const uint8_t *) &control;

    //ret = IOHIDDeviceSetReport(device, hrt, rid, payload, sizeof(RoccatControl));
    ret = IOHIDDeviceSetReportWithCallback( //
        device,
        hrt,
        rid,
        payload,
        payloadSize,
        timeout,             // Timeout in Sekunden
        writeReportCallback, // Callback-Funktion
        this                 // context
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

    return kIOReturnSuccess;
}

inline int RTHidDevice::writeDevice(IOHIDDeviceRef device, const quint8 *buffer, ssize_t length)
{
    IOReturn ret;
    IOHIDReportType hrt;
    quint8 ep = TYON_INTERFACE_MOUSE;

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

    m_isCBComplete = false;

    const quint8 reportId = buffer[0];
    const CFTimeInterval timeout = 2.0f;
    const uint8_t *payload = (const uint8_t *) &buffer[1];
    //ret = IOHIDDeviceSetReport(device, hrt, buffer[0], payload, length);
    ret = IOHIDDeviceSetReportWithCallback( //
        device,
        hrt,
        reportId,
        payload,
        length,
        timeout,             // Timeout in Sekunden
        writeReportCallback, // Callback-Funktion
        this                 // context
    );

    if (ret != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to write HID raw message. ret=%d", ret);
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

    return kIOReturnSuccess;
}

inline int RTHidDevice::selectProfileSettings(IOHIDDeviceRef device, uint pix)
{
    IOReturn ret;

    if (pix > TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    /* tyon_select(
     *  device: device,
     *  profile_index: profile_index,
     *  data_index: TYON_CONTROL_DATA_INDEX_NONE,
     *  request: TYON_CONTROL_REQUEST_PROFILE_BUTTONS,
     *  error)
     *
     * roccat_select(
     *  device: device,
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: data_index | profile_index,
     *  request: request,
     *  error)
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 PIX = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    const quint8 REQ = TYON_CONTROL_REQUEST_PROFILE_SETTINGS;

    if ((ret = writeHidReport(device, HEP, RID, PIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=%d", pix, ret);
        return ret;
    }

    qDebug("[HIDDEV] Profile settings selected. PIX=%d (0x%02x) REQ=%d (0x%02x)", PIX, PIX, REQ, REQ);
    return kIOReturnSuccess;
}

inline int RTHidDevice::selectProfileButtons(IOHIDDeviceRef device, uint pix)
{
    IOReturn ret;

    if (pix > TYON_PROFILE_NUM) {
        qCritical("[HIDDEV] Invalid profile index: %d", pix);
        return EINVAL;
    }

    /* tyon_select(
     *  device: device,
     *  profile_index: profile_index,
     *  data_index: TYON_CONTROL_DATA_INDEX_NONE,
     *  request: TYON_CONTROL_REQUEST_PROFILE_BUTTONS,
     *  error)
     *
     * roccat_select(
     *  device: device,
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: data_index | profile_index,
     *  request: request,
     *  error)
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 PIX = (TYON_CONTROL_DATA_INDEX_NONE | pix);
    const quint8 REQ = TYON_CONTROL_REQUEST_PROFILE_BUTTONS;

    if ((ret = writeHidReport(device, HEP, RID, PIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select profile %d. ret=%d", pix, ret);
        return ret;
    }

    qDebug("[HIDDEV] Profile buttons selected. PIX=%d (0x%02x) REQ=%d (0x%02x)", PIX, PIX, REQ, REQ);
    return kIOReturnSuccess;
}

inline int RTHidDevice::selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix)
{
    IOReturn ret;
    /* tyon_select(
     *  device: device,
     *  profile_index: profile_index,
     *  data_index: TYON_CONTROL_DATA_INDEX_MACRO_1 or 2,
     *  request: bix,
     *  error)
     *
     * roccat_select(
     *  device: device,
     *  endpoint: TYON_INTERFACE_MOUSE,
     *  report_id: TYON_REPORT_ID_CONTROL,
     *  profile_index: data_index | profile_index,
     *  request: request,
     *  error)
     */
    const quint8 HEP = TYON_INTERFACE_MOUSE;
    const quint8 RID = TYON_REPORT_ID_CONTROL;
    const quint8 PIX = (dix | pix);
    const quint8 REQ = bix;

    if ((ret = writeHidReport(device, HEP, RID, PIX, REQ)) != kIOReturnSuccess) {
        qCritical("[HIDDEV] Unable to select macro %d. ret=%d", REQ, ret);
        return ret;
    }

    qDebug("[HIDDEV] Macro selected. PIX=%d (0x%02x) REQ=%d (0x%02x)", PIX, PIX, REQ, REQ);
    return kIOReturnSuccess;
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

    if ((ret = readHidReport(device, TYON_REPORT_ID_MACRO, sizeof(TyonMacro1))) != kIOReturnSuccess) {
        return ret;
    }

    if ((ret = selectMacro(device, pix, dix2, bix))) {
        return ret;
    }

    if ((ret = readHidReport(device, TYON_REPORT_ID_MACRO, sizeof(TyonMacro2))) != kIOReturnSuccess) {
        return ret;
    }

    return kIOReturnSuccess;
}

inline int RTHidDevice::deviceReset(IOHIDDeviceRef device)
{
    TyonInfo info = {};
    info.report_id = TYON_REPORT_ID_INFO;
    info.function = TYON_INFO_FUNCTION_RESET;
    info.size = sizeof(TyonInfo);
    if (writeDevice(device, (const quint8 *) &info, sizeof(info))) {
        return EIO;
    }
    return kIOReturnSuccess;
}

#if 0
gboolean tyon_xcelerator_calibration_start(RoccatDevice *device, GError **error) {
    TyonInfo info = { 0 };
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_START;
    return tyon_info_write(device, &info, error);
}

gboolean tyon_xcelerator_calibration_data(RoccatDevice *device, guint8 min, guint8 mid, guint8 max, GError **error) {
    TyonInfo info = { 0 };
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_DATA;
    info.xcelerator_min = min;
    info.xcelerator_mid = mid;
    info.xcelerator_max = max;
    return tyon_info_write(device, &info, error);
}

gboolean tyon_xcelerator_calibration_end(RoccatDevice *device, GError **error) {
    TyonInfo info = { 0 };
    info.function = TYON_INFO_FUNCTION_XCELERATOR_CALIB_END;
    return tyon_info_write(device, &info, error);
}


gboolean tyon_info_write(RoccatDevice *device, TyonInfo *info, GError **error) {
    tyon_info_finalize(info);
    return tyon_device_write(device, (gchar const *)info, sizeof(TyonInfo), error);
}

gboolean tyon_reset(RoccatDevice *device, GError **error) {
    TyonInfo info = { 0 };
    info.function = TYON_INFO_FUNCTION_RESET;
    return tyon_info_write(device, &info, error);
}

guint tyon_firmware_version_read(RoccatDevice *device, GError **error) {
    TyonInfo *info;
    guint result;

    info = tyon_info_read(device, error);
    if (!info)
        return kIOReturnSuccess;
    result = info->firmware_version;
    g_free(info);
    return result;
}

TyonMacro *tyon_macro_read(RoccatDevice *tyon, guint profile_index, guint button_index, GError **error) {
    TyonMacro1 *macro1;
    TyonMacro2 *macro2;
    TyonMacro *macro;

    g_assert(profile_index < TYON_PROFILE_NUM);

    gaminggear_device_lock(GAMINGGEAR_DEVICE(tyon));

    if (!tyon_select(tyon, profile_index, TYON_CONTROL_DATA_INDEX_MACRO_1, button_index, error)) {
        gaminggear_device_unlock(GAMINGGEAR_DEVICE(tyon));
        return NULL;
    }

    macro1 = (TyonMacro1 *)tyon_device_read(tyon, TYON_REPORT_ID_MACRO, sizeof(TyonMacro1), error);
    if (!macro1) {
        gaminggear_device_unlock(GAMINGGEAR_DEVICE(tyon));
        return NULL;
    }

    if (!tyon_select(tyon, profile_index, TYON_CONTROL_DATA_INDEX_MACRO_2, button_index, error)) {
        gaminggear_device_unlock(GAMINGGEAR_DEVICE(tyon));
        g_free(macro1);
        return NULL;
    }

    macro2 = (TyonMacro2 *)tyon_device_read(tyon, TYON_REPORT_ID_MACRO, sizeof(TyonMacro2), error);
    if (!macro2) {
        gaminggear_device_unlock(GAMINGGEAR_DEVICE(tyon));
        g_free(macro1);
        return NULL;
    }

    gaminggear_device_unlock(GAMINGGEAR_DEVICE(tyon));

    macro = (TyonMacro *)g_malloc(sizeof(TyonMacro));
    memcpy(macro, macro1->data, TYON_MACRO_1_DATA_SIZE);
    memcpy((guint8 *)macro + TYON_MACRO_1_DATA_SIZE, macro2->data, TYON_MACRO_2_DATA_SIZE);

    g_free(macro1);
    g_free(macro2);

    return macro;
}



TyonControlUnit *tyon_control_unit_read(RoccatDevice *tyon, GError **error) {
    return (TyonControlUnit *)tyon_device_read(tyon, TYON_REPORT_ID_CONTROL_UNIT, sizeof(TyonControlUnit), error);
}

static gboolean tyon_control_unit_write(RoccatDevice *tyon, TyonControlUnit const *data, GError **error) {
    return tyon_device_write(tyon, (char const *)data, sizeof(TyonControlUnit), error);
}

gboolean tyon_tracking_control_unit_test(RoccatDevice *tyon, guint dcu, guint median, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = dcu;
    control_unit.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control_unit.median = median;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_CANCEL;

    return tyon_control_unit_write(tyon, &control_unit, error);
}

gboolean tyon_tracking_control_unit_cancel(RoccatDevice *tyon, guint dcu, GError **error) {
    return tyon_tracking_control_unit_test(tyon, dcu, 0, error);
}

gboolean tyon_tracking_control_unit_accept(RoccatDevice *tyon, guint dcu, guint median, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = dcu;
    control_unit.tcu = TYON_TRACKING_CONTROL_UNIT_ON;
    control_unit.median = median;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;

    return tyon_control_unit_write(tyon, &control_unit, error);
}

gboolean tyon_tracking_control_unit_off(RoccatDevice *tyon, guint dcu, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = dcu;
    control_unit.tcu = TYON_TRACKING_CONTROL_UNIT_OFF;
    control_unit.median = 0;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_OFF;

    return tyon_control_unit_write(tyon, &control_unit, error);
}

guint tyon_distance_control_unit_get(RoccatDevice *tyon, GError **error) {
    TyonControlUnit *control_unit;
    guint retval;

    control_unit = tyon_control_unit_read(tyon, error);
    if (!control_unit)
        return 0;

    retval = control_unit->dcu;
    g_free(control_unit);
    return retval;
}

gboolean tyon_distance_control_unit_try(RoccatDevice *tyon, guint new_dcu, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = new_dcu;
    control_unit.tcu = 0xff;
    control_unit.median = 0xff;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_UNDEFINED;

    return tyon_control_unit_write(tyon, &control_unit, error);
}

gboolean tyon_distance_control_unit_cancel(RoccatDevice *tyon, guint old_dcu, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = old_dcu;
    control_unit.tcu = 0xff;
    control_unit.median = 0xff;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_CANCEL;

    return tyon_control_unit_write(tyon, &control_unit, error);
}

gboolean tyon_distance_control_unit_accept(RoccatDevice *tyon, guint new_dcu, GError **error) {
    TyonControlUnit control_unit;

    control_unit.report_id = TYON_REPORT_ID_CONTROL_UNIT;
    control_unit.size = sizeof(TyonControlUnit);
    control_unit.dcu = new_dcu;
    control_unit.tcu = 0xff;
    control_unit.median = 0xff;
    control_unit.action = TYON_CONTROL_UNIT_ACTION_ACCEPT;

    return tyon_control_unit_write(tyon, &control_unit, error);
}
#endif
