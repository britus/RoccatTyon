#pragma once
/*
 * This file is part of roccat-tools.
 *
 * roccat-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * roccat-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with roccat-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include <QObject>

#define USB_DEVICE_ID_VENDOR_ROCCAT 0x1e7d
#define USB_DEVICE_ID_ROCCAT_TYON_BLACK 0x2e4a
#define USB_DEVICE_ID_ROCCAT_TYON_WHITE 0x2e4b

#define TYON_DEVICE_NAME "Tyon"
#define TYON_DEVICE_NAME_BLACK "Tyon Black"
#define TYON_DEVICE_NAME_WHITE "Tyon White"
#define TYON_DEVICE_NAME_COMBINED "Tyon Black/White"

typedef enum {
    ROCCAT_NIBBLE_HIGH = 1,
    ROCCAT_NIBBLE_LOW = 0,
} RoccatNibble;

enum {
    TYON_PROFILE_NUM = 5,
    TYON_LIGHTS_NUM = 2,
    TYON_CPI_MIN = 200,
    TYON_CPI_MAX = 8200,
    TYON_CPI_STEP = 200,
};

typedef enum {
    TYON_INFO_FUNCTION_RESET = 0x1,
    TYON_INFO_FUNCTION_XCELERATOR_CALIB_START = 0x8,
    TYON_INFO_FUNCTION_XCELERATOR_CALIB_DATA = 0xb,
    TYON_INFO_FUNCTION_XCELERATOR_CALIB_END = 0xa,
} TyonInfoFunction;

typedef enum {
    TYON_CONTROL_REQUEST_CHECK = 0x00,
    /* button indexes for requesting macros */
    TYON_CONTROL_REQUEST_PROFILE_SETTINGS = 0x80,
    TYON_CONTROL_REQUEST_PROFILE_BUTTONS = 0x90,
} TyonControlRequest;

typedef enum {
    TYON_CONTROL_DATA_INDEX_NONE = 0x00,
    TYON_CONTROL_DATA_INDEX_MACRO_1 = 0x10,
    TYON_CONTROL_DATA_INDEX_MACRO_2 = 0x20,
} TyonControlDataIndex;

typedef enum {
    TYON_REPORT_ID_SPECIAL = 0x03,
    TYON_REPORT_ID_CONTROL = 0x04,          /* 3 */
    TYON_REPORT_ID_PROFILE = 0x05,          /* 3 */
    TYON_REPORT_ID_PROFILE_SETTINGS = 0x06, /* 30 */
    TYON_REPORT_ID_PROFILE_BUTTONS = 0x07,  /* 99 */
    TYON_REPORT_ID_MACRO = 0x08,            /* 2002 */
    TYON_REPORT_ID_INFO = 0x09,             /* 8 (Firmware release)*/
    TYON_REPORT_ID_A = 0x0a,                /* 8 */
    TYON_REPORT_ID_SENSOR = 0x0c,           /* 4 */
    TYON_REPORT_ID_SROM_WRITE = 0x0d,       /* 1028 */
    TYON_REPORT_ID_DEVICE_STATE = 0x0e,     /* 3 */
    TYON_REPORT_ID_CONTROL_UNIT = 0x0f,     /* 6 */
    TYON_REPORT_ID_TALK = 0x10,             /* 16 */
    TYON_REPORT_ID_11 = 0x11,               /* 16 */
    TYON_REPORT_ID_1A = 0x1a,               /* 1029 */
} TyonReportId;

typedef enum {
    TYON_INTERFACE_MOUSE = 0,
    TYON_INTERFACE_KEYBOARD = 1,
    TYON_INTERFACE_JOYSTICK = 2,
    TYON_INTERFACE_MISC = 3,
} TyonInterface;

enum {
    TYON_PHYSICAL_BUTTON_NUM = 16,
    TYON_PROFILE_BUTTON_NUM = 32,
};

typedef enum {
    TYON_BUTTON_INDEX_LEFT = 0,
    TYON_BUTTON_INDEX_RIGHT,
    TYON_BUTTON_INDEX_MIDDLE,
    TYON_BUTTON_INDEX_THUMB_BACK,
    TYON_BUTTON_INDEX_THUMB_FORWARD,
    TYON_BUTTON_INDEX_THUMB_PEDAL,
    TYON_BUTTON_INDEX_THUMB_PADDLE_UP,
    TYON_BUTTON_INDEX_THUMB_PADDLE_DOWN,
    TYON_BUTTON_INDEX_LEFT_BACK,
    TYON_BUTTON_INDEX_LEFT_FORWARD,
    TYON_BUTTON_INDEX_RIGHT_BACK,
    TYON_BUTTON_INDEX_RIGHT_FORWARD,
    TYON_BUTTON_INDEX_FIN_RIGHT,
    TYON_BUTTON_INDEX_FIN_LEFT,
    TYON_BUTTON_INDEX_WHEEL_UP,
    TYON_BUTTON_INDEX_WHEEL_DOWN,
    // EasyShift //
    TYON_BUTTON_INDEX_SHIFT_LEFT,
    TYON_BUTTON_INDEX_SHIFT_RIGHT,
    TYON_BUTTON_INDEX_SHIFT_MIDDLE,
    TYON_BUTTON_INDEX_SHIFT_THUMB_BACK,
    TYON_BUTTON_INDEX_SHIFT_THUMB_FORWARD,
    TYON_BUTTON_INDEX_SHIFT_THUMB_PEDAL,
    TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_UP,
    TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_DOWN,
    TYON_BUTTON_INDEX_SHIFT_LEFT_BACK,
    TYON_BUTTON_INDEX_SHIFT_LEFT_FORWARD,
    TYON_BUTTON_INDEX_SHIFT_RIGHT_BACK,
    TYON_BUTTON_INDEX_SHIFT_RIGHT_FORWARD,
    TYON_BUTTON_INDEX_SHIFT_FIN_RIGHT,
    TYON_BUTTON_INDEX_SHIFT_FIN_LEFT,
    TYON_BUTTON_INDEX_SHIFT_WHEEL_UP,
    TYON_BUTTON_INDEX_SHIFT_WHEEL_DOWN,
} TyonButtonIndex;

typedef enum {
    TYON_BUTTON_TYPE_UNUSED = 0x00,
    TYON_BUTTON_TYPE_CLICK = 0x01,
    TYON_BUTTON_TYPE_MENU = 0x02,
    TYON_BUTTON_TYPE_UNIVERSAL_SCROLLING = 0x03,
    TYON_BUTTON_TYPE_DOUBLE_CLICK = 0x04,
    TYON_BUTTON_TYPE_SHORTCUT = 0x05,
    TYON_BUTTON_TYPE_DISABLED = 0x06,
    TYON_BUTTON_TYPE_BROWSER_FORWARD = 0x07,
    TYON_BUTTON_TYPE_BROWSER_BACKWARD = 0x08,
    TYON_BUTTON_TYPE_TILT_LEFT = 0x09,
    TYON_BUTTON_TYPE_TILT_RIGHT = 0x0a,
    TYON_BUTTON_TYPE_SCROLL_UP = 0x0d,
    TYON_BUTTON_TYPE_SCROLL_DOWN = 0x0e,
    TYON_BUTTON_TYPE_QUICKLAUNCH = 0x0f,
    TYON_BUTTON_TYPE_PROFILE_CYCLE = 0x10,
    TYON_BUTTON_TYPE_PROFILE_UP = 0x11,
    TYON_BUTTON_TYPE_PROFILE_DOWN = 0x12,
    TYON_BUTTON_TYPE_CPI_CYCLE = 0x14,
    TYON_BUTTON_TYPE_CPI_UP = 0x15,
    TYON_BUTTON_TYPE_CPI_DOWN = 0x16,
    TYON_BUTTON_TYPE_SENSITIVITY_CYCLE = 0x17,
    TYON_BUTTON_TYPE_SENSITIVITY_UP = 0x18,
    TYON_BUTTON_TYPE_SENSITIVITY_DOWN = 0x19,
    TYON_BUTTON_TYPE_WINDOWS_KEY = 0x1a,
    TYON_BUTTON_TYPE_OPEN_DRIVER = 0x1b,
    TYON_BUTTON_TYPE_OPEN_PLAYER = 0x20,
    TYON_BUTTON_TYPE_PREV_TRACK = 0x21,
    TYON_BUTTON_TYPE_NEXT_TRACK = 0x22,
    TYON_BUTTON_TYPE_PLAY_PAUSE = 0x23,
    TYON_BUTTON_TYPE_STOP = 0x24,
    TYON_BUTTON_TYPE_MUTE = 0x25,
    TYON_BUTTON_TYPE_VOLUME_DOWN = 0x26,
    TYON_BUTTON_TYPE_VOLUME_UP = 0x27,
    TYON_BUTTON_TYPE_MACRO = 0x30,
    TYON_BUTTON_TYPE_TIMER = 0x31,
    TYON_BUTTON_TYPE_TIMER_STOP = 0x32,
    TYON_BUTTON_TYPE_EASYAIM_1 = 0x33,
    TYON_BUTTON_TYPE_EASYAIM_2 = 0x34,
    TYON_BUTTON_TYPE_EASYAIM_3 = 0x35,
    TYON_BUTTON_TYPE_EASYAIM_4 = 0x36,
    TYON_BUTTON_TYPE_EASYAIM_5 = 0x37,
    TYON_BUTTON_TYPE_EASYSHIFT_SELF = 0x41, // FIXME confirm, firmware suggests it's 0x54
    TYON_BUTTON_TYPE_EASYWHEEL_SENSITIVITY = 0x42,
    TYON_BUTTON_TYPE_EASYWHEEL_PROFILE = 0x43,
    TYON_BUTTON_TYPE_EASYWHEEL_CPI = 0x44,
    TYON_BUTTON_TYPE_EASYWHEEL_VOLUME = 0x45,
    TYON_BUTTON_TYPE_EASYWHEEL_ALT_TAB = 0x46,
    TYON_BUTTON_TYPE_EASYWHEEL_AERO_FLIP_3D = 0x47,
    TYON_BUTTON_TYPE_EASYSHIFT_OTHER = 0x51,
    TYON_BUTTON_TYPE_EASYSHIFT_LOCK_OTHER = 0x52,
    TYON_BUTTON_TYPE_EASYSHIFT_ALL = 0x53,
    TYON_BUTTON_TYPE_XINPUT_1 = 0x60,
    TYON_BUTTON_TYPE_XINPUT_2 = 0x61,
    TYON_BUTTON_TYPE_XINPUT_3 = 0x62,
    TYON_BUTTON_TYPE_XINPUT_4 = 0x63,
    TYON_BUTTON_TYPE_XINPUT_5 = 0x64,
    TYON_BUTTON_TYPE_XINPUT_6 = 0x65,
    TYON_BUTTON_TYPE_XINPUT_7 = 0x66,
    TYON_BUTTON_TYPE_XINPUT_8 = 0x67,
    TYON_BUTTON_TYPE_XINPUT_9 = 0x68,
    TYON_BUTTON_TYPE_XINPUT_10 = 0x69,
    TYON_BUTTON_TYPE_XINPUT_RX_UP = 0x6a,
    TYON_BUTTON_TYPE_XINPUT_RX_DOWN = 0x6b,
    TYON_BUTTON_TYPE_XINPUT_RY_UP = 0x6c,
    TYON_BUTTON_TYPE_XINPUT_RY_DOWN = 0x6d,
    TYON_BUTTON_TYPE_XINPUT_X_UP = 0x6e,
    TYON_BUTTON_TYPE_XINPUT_X_DOWN = 0x6f,
    TYON_BUTTON_TYPE_XINPUT_Y_UP = 0x70,
    TYON_BUTTON_TYPE_XINPUT_Y_DOWN = 0x71,
    TYON_BUTTON_TYPE_XINPUT_Z_UP = 0x72,
    TYON_BUTTON_TYPE_XINPUT_Z_DOWN = 0x73,
    TYON_BUTTON_TYPE_DINPUT_1 = 0x74,
    TYON_BUTTON_TYPE_DINPUT_2 = 0x75,
    TYON_BUTTON_TYPE_DINPUT_3 = 0x76,
    TYON_BUTTON_TYPE_DINPUT_4 = 0x77,
    TYON_BUTTON_TYPE_DINPUT_5 = 0x78,
    TYON_BUTTON_TYPE_DINPUT_6 = 0x79,
    TYON_BUTTON_TYPE_DINPUT_7 = 0x7a,
    TYON_BUTTON_TYPE_DINPUT_8 = 0x7b,
    TYON_BUTTON_TYPE_DINPUT_9 = 0x7c,
    TYON_BUTTON_TYPE_DINPUT_10 = 0x7d,
    TYON_BUTTON_TYPE_DINPUT_11 = 0x7e,
    TYON_BUTTON_TYPE_DINPUT_12 = 0x7f,
    TYON_BUTTON_TYPE_DINPUT_X_UP = 0x80,
    TYON_BUTTON_TYPE_DINPUT_X_DOWN = 0x81,
    TYON_BUTTON_TYPE_DINPUT_Y_UP = 0x82,
    TYON_BUTTON_TYPE_DINPUT_Y_DOWN = 0x83,
    TYON_BUTTON_TYPE_DINPUT_Z_UP = 0x84,
    TYON_BUTTON_TYPE_DINPUT_Z_DOWN = 0x85,
    TYON_BUTTON_TYPE_HOME = 0x86,
    TYON_BUTTON_TYPE_END = 0x87,
    TYON_BUTTON_TYPE_PAGE_UP = 0x88,
    TYON_BUTTON_TYPE_PAGE_DOWN = 0x89,
    TYON_BUTTON_TYPE_L_CTRL = 0x8a,
    TYON_BUTTON_TYPE_L_ALT = 0x8b,
} TyonButtonType;

struct _TyonLight
{
    quint8 index;
    quint8 red;
    quint8 green;
    quint8 blue;
    quint8 unused;
} __attribute__((packed));
typedef struct _TyonLight TyonLight;

enum {
    TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM = 5,
};

struct _TyonProfile
{
    quint8 report_id; /* TYON_REPORT_ID_PROFILE */
    quint8 size;      /* always 0x03 */
    quint8 profile_index;
} __attribute__((packed));
typedef struct _TyonProfile TyonProfile;

typedef enum {
    TYON_LIGHT_WHEEL = 0,
    TYON_LIGHT_BOTTOM,
} TyonLightType;

struct _TyonProfileSettings
{
    quint8 report_id;            /* TYON_REPORT_ID_PROFILE_SETTINGS */
    quint8 size;                 /* always 30 */
    quint8 profile_index;        /* range 0-4 */
    quint8 advanced_sensitivity; /* RoccatSensitivityAdvanced */
    quint8 sensitivity_x;        /* RoccatSensitivity */
    quint8 sensitivity_y;        /* RoccatSensitivity */
    quint8 cpi_levels_enabled;
    quint8 cpi_levels[TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM];
    quint8 cpi_active;          /* range 0-4 */
    quint8 talkfx_polling_rate; /* TyonProfileSettingsTalkfx + RoccatPollingRate */
    quint8 lights_enabled;
    quint8 color_flow;
    quint8 light_effect;
    quint8 effect_speed;
    TyonLight lights[TYON_LIGHTS_NUM];
    quint16 checksum;
} __attribute__((packed));
typedef struct _TyonProfileSettings TyonProfileSettings;

struct _TyonControlUnit
{
    quint8 report_id; /* TYON_REPORT_ID_CONTROL_UNIT */
    quint8 size;      /* always 6 */
    quint8 dcu;
    quint8 tcu;
    quint8 median;
    quint8 action;
} __attribute__((packed));
typedef struct _TyonControlUnit TyonControlUnit;

typedef enum {
    TYON_CONTROL_UNIT_ACTION_CANCEL = 0x00,
    TYON_CONTROL_UNIT_ACTION_ACCEPT = 0x01,
    TYON_CONTROL_UNIT_ACTION_OFF = 0x02,
    TYON_CONTROL_UNIT_ACTION_UNDEFINED = 0xff,
} TyonControlUnitAction;

typedef enum {
    TYON_TRACKING_CONTROL_UNIT_OFF = 0,
    TYON_TRACKING_CONTROL_UNIT_ON = 1,
} TyonControlUnitTcu;

typedef enum {
    TYON_DISTANCE_CONTROL_UNIT_OFF = 0,
    TYON_DISTANCE_CONTROL_UNIT_EXTRA_LOW = 1,
    TYON_DISTANCE_CONTROL_UNIT_LOW = 2,
    TYON_DISTANCE_CONTROL_UNIT_NORMAL = 3,
} TyonControlUnitDcu;

struct _TyonInfo
{
    quint8 report_id; /* TYON_REPORT_ID_INFO */
    quint8 size;      /* always 0x08 */
    union {
        quint8 firmware_version; /* r */
        quint8 function;         /* w */
    };
    union {
        quint8 dfu_version;    /* r */
        quint8 xcelerator_min; /* w */
    };
    quint8 xcelerator_mid;
    quint8 xcelerator_max;
    quint8 unused1;
    quint8 unused2;
} __attribute__((packed));
typedef struct _TyonInfo TyonInfo;

struct _RoccatButton
{
    quint8 type;
    /* Shortcut uses modifier and key.
       Custom DPI stores dpi in modifier. */
    quint8 modifier;
    quint8 key;
} __attribute__((packed));
typedef struct _RoccatButton RoccatButton;

typedef enum {
    ROCCAT_BUTTON_MODIFIER_BIT_NONE = 0x0,
    ROCCAT_BUTTON_MODIFIER_BIT_SHIFT = 0x2, //EoF-BUGFIX: 0x01 -> 0x02
    ROCCAT_BUTTON_MODIFIER_BIT_CTRL = 0x4,  //EoF-BUGFIX: 0x02 -> 0x04
    ROCCAT_BUTTON_MODIFIER_BIT_ALT = 0x8,   //EoF-BUGFIX: 0x03 -> 0x08
    ROCCAT_BUTTON_MODIFIER_BIT_WIN = 0x10,  //EoF-BUGFIX: 0x04 -> 0x10
} RoccatButtonModifierBit;

struct _TyonProfileButtons
{
    quint8 report_id;     /* TYON_REPORT_ID_PROFILE_BUTTONS */
    quint8 size;          /* always 99 */
    quint8 profile_index; /* range 0-4 */
    RoccatButton buttons[TYON_PROFILE_BUTTON_NUM];
} __attribute__((packed));
typedef struct _TyonProfileButtons TyonProfileButtons;

struct _RoccatLight
{
    quint8 index; // color selector on device
    quint8 red;   // custom color (TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR)
    quint8 green; // custom color
    quint8 blue;  // custom color
} __attribute__((packed));
typedef struct _RoccatLight RoccatLight;

typedef enum {
    ROCCAT_TALK_DEVICE_NONE = 0,
    /* Devices use USB_DEVICE_ID_ROCCAT_* */
    ROCCAT_TALK_DEVICE_KEYBOARD = 0xfffd, /* np */
    ROCCAT_TALK_DEVICE_MOUSE = 0xfffe,    /* np */
    ROCCAT_TALK_DEVICE_ALL = 0xffff,
} RoccatTalkDevice;

typedef enum {
    ROCCAT_TALK_EASYAIM_OFF = 0,
    ROCCAT_TALK_EASYAIM_1 = 1,
    ROCCAT_TALK_EASYAIM_2 = 2,
    ROCCAT_TALK_EASYAIM_3 = 3,
    ROCCAT_TALK_EASYAIM_4 = 4,
    ROCCAT_TALK_EASYAIM_5 = 5,
} RoccatTalkEasyaim;

enum {
    ROCCAT_TIMER_NAME_LENGTH = 24,
};

struct _RoccatTimer
{
    /* name needs to be null terminated */
    quint8 name[ROCCAT_TIMER_NAME_LENGTH];
    quint32 seconds;
};
typedef struct _RoccatTimer RoccatTimer;
typedef struct _RoccatTimers RoccatTimers;

struct _RoccatControl
{
    quint8 report_id;
    quint8 value;
    quint8 request;
} __attribute__((packed));
typedef struct _RoccatControl RoccatControl;

typedef enum {
    ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_1 = 0,
    ROCCAT_CONTROL_VALUE_STATUS_OK = 1,
    ROCCAT_CONTROL_VALUE_STATUS_INVALID = 2,
    ROCCAT_CONTROL_VALUE_STATUS_BUSY = 3,
    ROCCAT_CONTROL_VALUE_STATUS_CRITICAL_2 = 4, /* used by Ryos MK */
} RoccatControlValue;

typedef enum {
    TYON_PROFILE_SETTINGS_TALKFX_ON = 0,
    TYON_PROFILE_SETTINGS_TALKFX_OFF = 1,
} TyonProfileSettingsTalkfx;

typedef enum {
    TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_WHEEL = 0x01, //EOF-BUTFIX: 0 -> 1
    TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_BOTTOM = 0x02,
    TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR = 0x10, //EOF-BUTFIX: 0x04 -> 0x10
} TyonProfileSettingsLightsEnabled;

typedef enum {
    TYON_PROFILE_SETTINGS_COLOR_FLOW_OFF = 0,
    TYON_PROFILE_SETTINGS_COLOR_FLOW_SIMULTANEOUSLY = 1,
    TYON_PROFILE_SETTINGS_COLOR_FLOW_UP = 2,
    TYON_PROFILE_SETTINGS_COLOR_FLOW_DOWN = 3,
} TyonProfileSettingsColorFlow;

typedef enum {
    TYON_PROFILE_SETTINGS_LIGHT_EFFECT_ALL_OFF = 0,
    TYON_PROFILE_SETTINGS_LIGHT_EFFECT_FULLY_LIGHTED = 1,
    TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BLINKING = 2,
    TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BREATHING = 3,
    TYON_PROFILE_SETTINGS_LIGHT_EFFECT_HEARTBEAT = 4,
} TyonProfileSettingsLightEffect;

typedef enum {
    TYON_PROFILE_SETTINGS_EFFECT_SPEED_MIN = 1,
    TYON_PROFILE_SETTINGS_EFFECT_SPEED_MAX = 3,
} TyonProfileSettingsEffectSpeed;

typedef enum {
    ROCCAT_SENSITIVITY_ADVANCED_OFF = 0,
    ROCCAT_SENSITIVITY_ADVANCED_ON = 1,
} RoccatSensitivityAdvanced;

typedef enum {
    ROCCAT_POLLING_RATE_125 = 0,
    ROCCAT_POLLING_RATE_250 = 1,
    ROCCAT_POLLING_RATE_500 = 2,
    ROCCAT_POLLING_RATE_1000 = 3,
} RoccatPollingRate;

typedef enum {
    ROCCAT_SENSITIVITY_MIN = 0x01,
    ROCCAT_SENSITIVITY_CENTER = 0x06,
    ROCCAT_SENSITIVITY_MAX = 0x0b,
} RoccatSensitivity;

enum {
    TYON_MACRO_MACROSET_NAME_LENGTH = 24,
    TYON_MACRO_MACRO_NAME_LENGTH = 24,
    TYON_MACRO_KEYSTROKES_NUM = 480,
};

typedef struct _RoccatKeystroke RoccatKeystroke;

struct _RoccatKeystroke
{
    quint8 key;
    quint8 action;
    quint16 period; /*!< in milliseconds */
} __attribute__((packed));

/* This structure is transferred to hardware in 2 parts */
struct _TyonMacro
{
    quint8 profile_index;
    quint8 button_index;
    quint8 loop;
    quint8 unused1[24];
    quint8 macroset_name[TYON_MACRO_MACROSET_NAME_LENGTH];
    quint8 macro_name[TYON_MACRO_MACRO_NAME_LENGTH];
    quint16 count;
    RoccatKeystroke keystrokes[TYON_MACRO_KEYSTROKES_NUM];
} __attribute__((packed));

typedef struct _TyonMacro TyonMacro;
typedef struct _TyonMacro1 TyonMacro1;
typedef struct _TyonMacro2 TyonMacro2;

enum {
    TYON_MACRO_1_DATA_SIZE = 1024,
    TYON_MACRO_2_DATA_SIZE = sizeof(TyonMacro) - TYON_MACRO_1_DATA_SIZE,
    TYON_MACRO_2_UNUSED_SIZE = 1024 - TYON_MACRO_2_DATA_SIZE,
};

struct _TyonMacro1
{
    quint8 report_id;
    quint8 one;
    quint8 data[TYON_MACRO_1_DATA_SIZE];
} __attribute__((packed));

struct _TyonMacro2
{
    quint8 report_id;
    quint8 two;
    quint8 data[TYON_MACRO_2_DATA_SIZE];
    quint8 unused[TYON_MACRO_2_UNUSED_SIZE];
} __attribute__((packed));

struct _TyonDeviceState
{
    quint8 report_id; /* TYON_REPORT_ID_DEVICE_STATE */
    quint8 size;      /* always 0x03 */
    quint8 state;
} __attribute__((packed));
typedef struct _TyonDeviceState TyonDeviceState;

enum {
    TYON_LIGHT_INFO_COLORS_NUM = 16,
};

enum {
    TYON_SENSOR_IMAGE_SIZE = 30,
};

struct _TyonSensorImage
{
    quint8 report_id; /* TYON_REPORT_ID_SENSOR */
    quint8 action;    /* 3 */
    quint8 unused1;   /* 0 */
    quint8 data[TYON_SENSOR_IMAGE_SIZE * TYON_SENSOR_IMAGE_SIZE];
    quint8 unused2[125]; /* all bytes 0 */
} __attribute__((packed));
typedef struct _TyonSensorImage TyonSensorImage;

struct _TyonSensor
{
    quint8 report_id; /* TYON_REPORT_ID_SENSOR */
    quint8 action;
    quint8 reg;
    quint8 value;
} __attribute__((packed));
typedef struct _TyonSensor TyonSensor;

typedef enum {
    TYON_SENSOR_ACTION_WRITE = 1,
    TYON_SENSOR_ACTION_READ = 2,
    TYON_SENSOR_ACTION_FRAME_CAPTURE = 3,
} TyonSensorAction;

struct _TyonSpecial
{
    quint8 report_id; /* TYON_REPORT_ID_SPECIAL */
    quint8 analogue;
    quint8 type;
    quint8 data;
    quint8 action;
} __attribute__((packed));
typedef struct _TyonSpecial TyonSpecial;

typedef enum {
    TYON_SPECIAL_TYPE_TILT = 0x10,    /* data: 0xff = left, 0x01 = right */
    TYON_SPECIAL_TYPE_PROFILE = 0x20, /* data: profile_nr */
    /* gets sent on xcelerator down if set to shortcut */
    TYON_SPECIAL_TYPE_XCELERATOR = 0x25,             /* data: value, action = 0xa9 */
    TYON_SPECIAL_TYPE_QUICKLAUNCH = 0x60,            /* data: button_nr, action: press/release */
    TYON_SPECIAL_TYPE_TIMER_START = 0x80,            /* data: button_nr, action: press/release */
    TYON_SPECIAL_TYPE_TIMER_STOP = 0x90,             /* action: press/release */
    TYON_SPECIAL_TYPE_OPEN_DRIVER = 0xa0,            /* data: press/release ! */
    TYON_SPECIAL_TYPE_CPI = 0xb0,                    /* data: 1-5 */
    TYON_SPECIAL_TYPE_SENSITIVITY = 0xc0,            /* data: action: 1-b */
    TYON_SPECIAL_TYPE_ANALOGUE = 0xd1,               /* analogue: value, data: 0x00, action: 0x10 */
    TYON_SPECIAL_TYPE_XCELERATOR_CALIBRATION = 0xe0, /* data: 0x06, action: value */
    TYON_SPECIAL_TYPE_RAD_LEFT = 0xe1,               /* all rads: data: count */
    TYON_SPECIAL_TYPE_RAD_RIGHT = 0xe2,
    TYON_SPECIAL_TYPE_RAD_MIDDLE = 0xe3,
    TYON_SPECIAL_TYPE_RAD_THUMB_BACKWARD = 0xe4,
    TYON_SPECIAL_TYPE_RAD_THUMB_FORWARD = 0xe5,
    TYON_SPECIAL_TYPE_RAD_SCROLL_UP = 0xe6,
    TYON_SPECIAL_TYPE_RAD_SCROLL_DOWN = 0xe7,
    TYON_SPECIAL_TYPE_RAD_EASYSHIFT = 0xe8,
    TYON_SPECIAL_TYPE_RAD_EASYAIM = 0xe9,
    TYON_SPECIAL_TYPE_RAD_DISTANCE = 0xea,
    TYON_SPECIAL_TYPE_MULTIMEDIA = 0xf0, /* data: ?, action: press/release */
    TYON_SPECIAL_TYPE_TALK = 0xff,       /* data: button_nr, action: press/release */
} TyonSpecialType;

typedef enum {
    TYON_SPECIAL_ACTION_PRESS = 0x00,
    TYON_SPECIAL_ACTION_RELEASE = 0x01,
} TyonSpecialAction;

struct _TyonTalk
{
    quint8 report_id; /* TYON_REPORT_ID_TALK */
    quint8 size;      /* always 16 */
    quint8 easyshift;
    quint8 easyshift_lock;
    quint8 easyaim;
    quint8 fx_status;
    quint8 zone;
    quint8 unused;
    quint8 effect;
    quint8 speed;
    quint8 ambient_red;
    quint8 ambient_green;
    quint8 ambient_blue;
    quint8 event_red;
    quint8 event_green;
    quint8 event_blue;
} __attribute__((packed));
typedef struct _TyonTalk TyonTalk;

/* also valid as TyonTalkEasyshiftLock */
typedef enum {
    TYON_TALK_EASYSHIFT_OFF = 0,
    TYON_TALK_EASYSHIFT_ON = 1,
    TYON_TALK_EASYSHIFT_UNUSED = 0xff,
} TyonTalkEasyshift;

typedef enum {
    TYON_TALK_EASYAIM_OFF = 0,
    TYON_TALK_EASYAIM_1 = 1,
    TYON_TALK_EASYAIM_2 = 2,
    TYON_TALK_EASYAIM_3 = 3,
    TYON_TALK_EASYAIM_4 = 4,
    TYON_TALK_EASYAIM_5 = 5,
    TYON_TALK_EASYAIM_UNUSED = 0xff,
} TyonTalkEasyaim;

typedef enum {
    ROCCAT_TALKFX_STATE_OFF = 0x00,
    ROCCAT_TALKFX_STATE_ON = 0x01,
    TYON_TALKFX_STATE_UNUSED = 0xff,
} TyonTalkfxState;

typedef enum {
    TYON_TALKFX_ZONE_EVENT = 4,
    TYON_TALKFX_ZONE_AMBIENT = 3,
} TyonTalkfxZone;

typedef enum {
    ROCCAT_TALKFX_ZONE_AMBIENT = 0x01, //EoF-BUFIX: 0x00 -> 0x01
    ROCCAT_TALKFX_ZONE_EVENT = 0x02,   //EoF-BUFIX: 0x01 -> 0x02
    ROCCAT_TALKFX_ZONE_BIT_MASK = 0x00ff0000,
    ROCCAT_TALKFX_ZONE_BIT_SHIFT = 16,
} RoccatTalkfxZone;

typedef enum {
    ROCCAT_TALKFX_EFFECT_OFF = 0x00,
    ROCCAT_TALKFX_EFFECT_ON = 0x01,
    ROCCAT_TALKFX_EFFECT_BLINKING = 0x02,
    ROCCAT_TALKFX_EFFECT_BREATHING = 0x03,
    ROCCAT_TALKFX_EFFECT_HEARTBEAT = 0x04, /* not supported by IskuFX */
    ROCCAT_TALKFX_EFFECT_BIT_MASK = 0x0000ff00,
    ROCCAT_TALKFX_EFFECT_BIT_SHIFT = 8,
} RoccatTalkfxEffect;

typedef enum {
    ROCCAT_TALKFX_SPEED_OFF = 0x00,
    ROCCAT_TALKFX_SPEED_SLOW = 0x01,
    ROCCAT_TALKFX_SPEED_NORMAL = 0x02,
    ROCCAT_TALKFX_SPEED_FAST = 0x03,
    ROCCAT_TALKFX_SPEED_BIT_MASK = 0x000000ff,
    ROCCAT_TALKFX_SPEED_BIT_SHIFT = 0,
} RoccatTalkfxSpeed;

typedef enum {
    ROCCAT_TALKFX_COLOR_RED_MASK = 0x00ff0000,
    ROCCAT_TALKFX_COLOR_GREEN_MASK = 0x0000ff00,
    ROCCAT_TALKFX_COLOR_BLUE_MASK = 0x000000ff,
    ROCCAT_TALKFX_COLOR_RED_SHIFT = 16,
    ROCCAT_TALKFX_COLOR_GREEN_SHIFT = 8,
    ROCCAT_TALKFX_COLOR_BLUE_SHIFT = 0,
} RoccatTalkfxColor;
