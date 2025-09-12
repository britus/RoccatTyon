#pragma once
#include "rthiddevice.h"
#include "rttypes.h"
#include <QObject>

// -----------------------------------------------------
//  Debug stuff only
// -----------------------------------------------------

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
            qDebug("[HIDDEV] %s =  %s", //
                   qPrintable(keyStr),
                   qPrintable(QString::fromCFString((CFStringRef) value)));
        } else if (typeID == CFNumberGetTypeID()) {
            long num;
            CFNumberGetValue((CFNumberRef) value, kCFNumberLongType, &num);
            qDebug("[HIDDEV] %s = %ld (0x%lx)", //
                   qPrintable(keyStr),
                   num,
                   num);
        } else if (typeID == CFBooleanGetTypeID()) {
            bool b = CFBooleanGetValue((CFBooleanRef) value);
            qDebug("[HIDDEV] %s = %s", //
                   qPrintable(keyStr),
                   (b ? "true" : "false"));
        } else {
            // For unsupported types, use CFCopyDescription
            CFStringRef desc = CFCopyDescription(value);
            qDebug("[HIDDEV] %s = %s", //
                   qPrintable(keyStr),
                   qPrintable(QString::fromCFString(desc)));
            CFRelease(desc);
        }
    };

    qDebug("[Device found ----------------------------------------]");
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
        qDebug("[HIDDEV] SETTINGS: LIGHT-%d RED=0x%02x GREEN=0x%02x BLUE=0x%02x IDX=%d", //
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

static inline void debugTalkFx(TyonTalk *p)
{
    qDebug("[HIDDEV] TALKFX: SIZE=%d easyshift=0x%02x easyshift_lock=0x%02x", //
           p->size,
           p->easyshift,
           p->easyshift_lock);
    qDebug("[HIDDEV] TALKFX: easyaim=0x%02x effect=0x%02x", //
           p->easyaim,
           p->effect);
    qDebug("[HIDDEV] TALKFX: ambient_red=0x%02x ambient_green=0x%02x ambient_blue=0x%02x", //
           p->ambient_red,
           p->ambient_green,
           p->ambient_blue);
    qDebug("[HIDDEV] TALKFX: event_red=0x%02x event_green=0x%02x event_blue=0x%02x", //
           p->event_red,
           p->event_green,
           p->event_blue);
    qDebug("[HIDDEV] TALKFX: fx_status=0x%02x zone=0x%02x speed=0x%02x", //
           p->fx_status,
           p->zone,
           p->speed);
}

#endif
