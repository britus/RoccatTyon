// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once
#include "rtabstractdevice.h"
#include "rttypedefs.h"
#include <QAbstractItemModel>
#include <QColor>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QPushButton>

#define HIDAPI_MAX_STR 255

#ifndef CB_BIND
#define CB_BIND(o, x) std::bind(x, o, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
#endif

/**
 * @brief The RTController class manage the access to the ROCCAT Tyon mouse using the HID protocol
 */
class RTController : public QObject
{
    Q_OBJECT

public:
    /**
     * Callback function to assign ROCCAT Tyon button function
     */
    typedef std::function<void(TyonButtonIndex, TyonButtonType, QKeyCombination)> TSetButtonCallback;

    /**
     * Definition of ROCCAT Tyon physical button
     */
    typedef struct
    {
        QString name;
        TyonButtonIndex standard_index;
        TyonButtonIndex easyshift_index;
    } TPhysicalButton;

    /**
     * This type describe the link of physical button and
     * its assignment callback function TSetButtonCallback
     */
    typedef struct
    {
        TyonButtonIndex index;
        TSetButtonCallback handler;
    } TButtonLink;

    /**
     * Defines the ROCCAT Tyon profile
     */
    typedef struct
    {
        QString name;
        quint8 index;
        bool changed;
        TyonProfileSettings settings;
        TyonProfileButtons buttons;
    } TProfile;

    /**
     * Defines the ROCCAT device light colors
     */
    typedef struct
    {
        TyonLight deviceColors;
    } TColorItem;

    /**
     * ROCCAT Tyon profiles by profile index
     */
    typedef QMap<quint8, TProfile> TProfiles;

    /**
     * @brief ROCCAT Tyon color index to RGB mapping
     */
    typedef QMap<quint8, TColorItem> TDeviceColors;

    /**
     * @brief HID report response handler function
     */
    typedef std::function<int(const quint8 *, qsizetype)> TReportHandler;

    /**
     * @brief Default constructor
     * @param parent NULL or QObject
     */
    explicit RTController(QObject *parent = nullptr);

    /** */
    ~RTController();

    /**
     * @brief Return true if devices found
     * @return True or False
     */
    inline bool hasDevice() const { return (m_hid && m_hid->hasDevice()); }

    /**
     * @brief Return active profile index
     * @return Profile index
     */
    inline quint8 activeProfileIndex() const { return m_activeProfile.profile_index; }

    /**
     * @brief Return number of device profiles
     * @return 5 (TYON_PROFILE_NUM)
     */
    inline quint8 profileCount() const { return m_profiles.count(); }

    /**
     * @brief Return a profile by given profile index
     * @param pix Profile index 0-4
     * @param found Return true if found otherwise false
     * @return A profile structure
     */
    inline TProfile profile(quint8 pix, bool &found) const
    {
        if (pix > TYON_PROFILE_NUM) {
            found = false;
            return {};
        }
        if (!m_profiles.contains(pix)) {
            found = false;
            return {};
        }
        found = true;
        return m_profiles[pix];
    }

    /**
     * @brief Return active profile name
     * @return QString
     */
    QString profileName() const;

    /**
     * @brief Create button mapping for ROCCAT Tyon mouse button and UI push button
     * @param rb ROCCAT Tyon mouse button descriptor
     * @param button QT push button
     */
    void setupButton(const RoccatButton &rb, QPushButton *button);

    /**
     * @brief Assign ROCCAT Tyon button function
     * @param type Physical button
     * @param func Button function
     * @param QKeyCombination If short function, the shortcut
     */
    void assignButton(TyonButtonIndex type, TyonButtonType func, QKeyCombination kc);

    /**
     * @brief Translate ROCCAT Tyon shortcut to QT key sequence
     * @param button ROCCAT Tyon button structure
     * @return QKeySequence object
     */
    const QKeySequence toKeySequence(const RoccatButton &button) const;

    /**
     * @brief Convert Roccat sensitivity X value to UI value
     * @param settings A pointer to TyonProfileSettings
     * @return A value usable by UI
     */
    qint16 toSensitivityXValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert Roccat sensitivity Y value to UI value
     * @param settings A pointer to TyonProfileSettings
     * @return A value usable by UI
     */
    qint16 toSensitivityYValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert Roccat DPI level to UI value
     * @param settings A pointer to TyonProfileSettings
     * @return A value usable by UI
     */
    quint16 toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const;

    /**
     * @brief Translte QT color to ROCCAT Tyon light info
     * @param color QT color object
     * @param target ROCCAT Tyon light 0=Wheel 1=Bottom
     * @return TyonColorInfo structure
     */
    TyonLight toDeviceColor(TyonLightType target, const QColor &color) const;

    /**
     * @brief Translte ROCCAT Tyon light to UI color
     * @param light ROCCAT Tyon light info structure
     * @param isCustomColor True for custom color
     * @return QColor object
     */
    QColor toScreenColor(const TyonLight &light, bool isCustomColor = false) const;

    /**
     * @brief Return ROCCAT Tyon light color table
     * @return A mapping of color index to RGB
     */
    inline const TDeviceColors &deviceColors() const { return m_colors; }

    /**
     * @brief Return the descriptor map for the UI menu
     * @return QMap object
     */
    inline const QMap<quint8, QString> &buttonTypes() { return m_buttonTypes; }

    /**
     * @brief Set the color for the wheel or bottom light
     * @param target 0=Wheel or 1=Bottom
     * @param light Tyon light info type
     */
    void setLightColor(TyonLightType target, const TyonLight &light);

    /**
     * @brief Return the X-Celerate minimum
     * @return Numbers
     */
    quint8 minimumXCelerate() const;

    /**
     * @brief Return the X-Celerate maximum
     * @return Number
     */
    quint8 maximumXCelerate() const;

    /**
     * @brief Return the X-Celerate middle value
     * @return Number
     */
    quint8 middleXCelerate() const;

    /**
     * @brief Calculates the surface test median from image data
     * @param image The surface image
     * @return Number
     */
    uint sensorMedianOfImage(TyonSensorImage const *image);

    /**
     * @brief Return the TalkFX status
     * @param settings
     * @return True or false
     */
    bool talkFxState(const TyonProfileSettings *settings) const;

    /**
     * @brief Return the state of the distance control unit
     * @return TyonControlUnitDcu
     */
    TyonControlUnitDcu dcuState() const;

    /**
     * @brief Return the state of the tracking control unit
     * @return TyonControlUnitTcu
     */
    TyonControlUnitTcu tcuState() const;

    /**
     * @brief Return the median of the surface test
     * @return Number
     */
    uint tcuMedian() const;

    /**
     * @brief Return ROCCAT Tyon polling rate
     * @param Pointer to profile settings structure
     * @return Rate value
     */
    quint8 talkFxPollRate(const TyonProfileSettings *settings) const;

signals:
    void lookupStarted();
    void deviceWorkerStarted();
    void deviceWorkerFinished();
    void deviceFound();
    void deviceRemoved();
    void deviceError(int error, const QString &message);
    void deviceInfo(const TyonInfo &info);
    void profileIndexChanged(const quint8 pix);
    void profileChanged(const RTController::TProfile &profile);
    void controlUnitChanged(const TyonControlUnit &controlUnit);
    void sensorChanged(const TyonSensor &sensor);
    void sensorImageChanged(const TyonSensorImage &image);
    void sensorMedianChanged(int median);
    void specialReport(uint reportId, const QByteArray &report);
    void talkFxChanged(const TyonTalk &talkFx);

public slots:
    /**
     * @brief Find ROCCAT Tyon device
     */
    void lookupDevice();

    /**
     * @brief Save ROCCAT Tyon profiles to device
     * @return True if success
     */
    void updateDevice();

    /**
     * @brief Save all ROCCAT Tyon profiles to file
     * @param fileName The file name
     * @return True if success
     */
    void saveProfilesToFile(const QString &fileName);

    /**
     * @brief Load all ROCCAT Tyon profiles from file
     * @param fileName The file name
     * @param raiseEvents True to raise profile change event
     * @return True if success
     */
    void loadProfilesFromFile(const QString &fileName, bool raiseEvents = true);

    /**
     * @brief ROCCAT Tyon device to defaults
     * @return True if success
     */
    void resetProfiles();

    /**
     * @brief Set active profile index
     * @param index A Value of 0 - 4
     */
    void setActiveProfile(quint8 index);

    /**
     * @brief Set the profile name for given profile index
     * @param name The name of the profile
     * @param index A Value of 0 - 4
     */
    void setProfileName(const QString &name, quint8 profileIndex);

    /**
     * @brief Set the mouse X sensitivity
     * @param sensitivity
     */
    void setXSensitivity(qint16 sensitivity);

    /**
     * @brief Set the mouse Y sensitivity
     * @param sensitivity
     */
    void setYSensitivity(qint16 sensitivity);

    /**
     * @brief Set the advanced sensitivity mode
     * @param state True independent / false X/Y same value
     */
    void setAdvancedSenitivity(bool state);

    /**
     * @brief Set sensor poll rate
     * @param rate
     */
    void setTalkFxPollRate(quint8 rate);

    /**
     * @brief Enable / disable DPI slot
     * @param bit The ROCCAT Tyon DPI slot bit
     * @param state True enabled / False disabled
     */
    void setDpiSlot(quint8 bit, bool state);

    /**
     * @brief Set active DPI value by slot index
     * @param index The slot index
     */
    void setActiveDpiSlot(quint8 index);

    /**
     * @brief Set the DPI value by slot index
     * @param index The slot index
     * @param index DPI value 200 to 8200
     */
    void setDpiLevel(quint8 index, quint16 value);

    /**
     * @brief Enable ROCCAT Tyon wheel light
     * @param state True enable / False disable
     */
    void setLightWheelEnabled(bool state);

    /**
     * @brief Enable ROCCAT Tyon bottom light
     * @param state True enable / False disable
     */
    void setLightBottomEnabled(bool state);

    /**
     * @brief Enable custom color light
     * @param state True enable / False disable
     */
    void setLightCustomColorEnabled(bool state);

    /**
     * @brief Set the light effect mode
     * @param value Light effect mode
     */
    void setLightsEffect(quint8 value);

    /**
     * @brief Set the color flow mode
     * @param value Color flow mode
     */
    void setColorFlow(quint8 value);

    void setTalkFxState(bool state);
    void setDcuState(TyonControlUnitDcu state);
    void setTcuState(bool state);

    // X-Celerator calibration
    void xcStartCalibration();
    void xcStopCalibration();
    void xcApplyCalibration(quint8 min, quint8 mid, quint8 max);

    // TCU calibration
    void tcuSensorTest(TyonControlUnitDcu dcuState, uint median);
    void tcuSensorAccept(TyonControlUnitDcu dcuState, uint median);
    void tcuSensorCancel(TyonControlUnitDcu dcuState);
    void tcuSensorCaptureImage();
    void tcuSensorReadImage();
    int tcuSensorReadMedian(TyonSensorImage *image);

private slots:
    void onDeviceFound(THidDeviceType type);
    void onDeviceRemoved();
    void onLookupStarted();
    void onErrorOccured(int error, const QString &message);
    void onInputReady(quint32 rid, const QByteArray &data);

private:
    const uint kHIDUsageMouse = 0x04;
    const uint kHIDPageMouse = 0x00;
    const uint kHIDUsageMisc = 0x00;
    const uint kHIDPageMisc = 0x0a;

    typedef struct
    {
        QString transport;     // USB
        uint vendorId;         //  7805 (0x1e7d)
        uint vendorIdSource;   //  null
        uint productId;        //  11850 (0x2e4a)
        uint versionNumber;    //  256 (0x100)
        QString manufacturer;  //  ROCCAT
        QString product;       //  ROCCAT Tyon Black
        QString serialNumber;  //  ROC-11-850
        uint countryCode;      //  0 (0x0)
        uint locationId;       //  1048576 (0x100000)
        QString deviceUsage;   //  null
        uint primaryUsage;     //  Mouse = 0x04 or Misc = 0x00
        uint primaryUsagePage; //  Mouse = 0x01 or Misc = 0x0a
    } THidDeviceInfo;

    RTAbstractDevice *m_hid;
    TReportHandlers m_handlers;
    TDeviceColors m_colors;
    TyonInfo m_info;
    TyonProfile m_activeProfile;
    TProfiles m_profiles;
    TyonTalk m_talkFx;
    TyonSensor m_sensor;
    TyonSensorImage m_sensorImage;
    TyonControlUnit m_controlUnit;
    quint8 m_requestedProfile;
    bool m_initComplete;
    QMap<quint8, QString> m_buttonTypes;
    QMap<quint8, RTController::TPhysicalButton> m_physButtons;

private:
    inline void raiseError(int error, const QString &message);
    // --
    inline void initializeProfiles();
    inline void initializeColorMapping();
    inline void initializeHandlers();
    inline void initButtonTypes();
    inline void initPhysicalButtons();
    inline void setButtonType(const QString &name, quint8 type);
    inline void setPhysicalButton(quint8 index, TPhysicalButton pb);
    // --
    inline void internalSaveProfiles();
    // --
    inline void setModified(quint8 pix, bool changed);
    inline void setModified(TProfile *p, bool changed);
    inline void updateProfile(TProfile &p, bool changed);
    // get state of device
    inline bool roccatControlCheck();
    inline bool roccatControlWrite(uint pix, uint req);
    inline bool readDeviceControl();
    inline bool setDeviceState(bool state);
    // get firmware,DFU,X-Celerator min/max info
    inline bool readDeviceInfo();
    // get and set device profiles
    inline bool readActiveProfile();
    inline bool readProfiles(quint8 pix);
    inline bool selectProfileSettings(uint pix);
    inline bool readProfileSettings();
    inline bool selectProfileButtons(uint pix);
    inline bool readProfileButtons();
    // get and set button macros
    inline bool selectMacro(uint pix, uint dix, uint bix);
    inline bool readButtonMacro(uint pix, uint bix);
    // X-Celerator calibration
    inline bool xcCalibWriteStart();
    inline bool xcCalibWriteEnd();
    inline bool xcCalibWriteData(quint8 min, quint8 mid, quint8 max);
    // TCU calibration
    inline bool readControlUnit();
    inline bool tcuReadSensor();
    inline bool tcuReadSensorImage();
    inline bool tcuReadSensorRegister(quint8 reg);
    inline bool tcuWriteTest(quint8 dcuState, uint median);
    inline bool tcuWriteAccept(quint8 dcuState, uint median);
    inline bool tcuWriteCancel(quint8 dcuState);
    inline bool tcuWriteOff(quint8 dcuState);
    inline bool tcuWriteTry(quint8 dcuState);
    inline bool tcuWriteSensorCommand(quint8 action, quint8 reg, quint8 value);
    inline bool tcuWriteSensorRegister(quint8 reg, quint8 value);
    inline bool tcuWriteSensorImageCapture();
    // --
    inline bool dcuWriteState(quint8 dcuState);
    // --
    inline bool talkRead();
    inline bool talkWriteReport(TyonTalk *talk);
    inline bool talkWriteKey(quint8 easyshift, quint8 easyshift_lock, quint8 easyaim);
    inline bool talkWriteEasyshift(quint8 state);
    inline bool talkWriteEasyshiftLock(quint8 state);
    inline bool talkWriteEasyAim(quint8 state);
    inline bool talkWriteFxData(TyonTalk *tyonTalk);
    inline bool talkWriteFx(quint32 effect, quint32 ambient_color, quint32 event_color);
    inline bool talkWriteFxState(quint8 state);
};

Q_DECLARE_METATYPE(TyonInfo);
Q_DECLARE_METATYPE(TyonLight);
Q_DECLARE_METATYPE(TyonProfileSettings);
Q_DECLARE_METATYPE(TyonProfileButtons);
Q_DECLARE_METATYPE(TyonButtonIndex);
Q_DECLARE_METATYPE(TyonButtonType);
Q_DECLARE_METATYPE(TyonControlUnit);
Q_DECLARE_METATYPE(TyonSensor);
Q_DECLARE_METATYPE(TyonSensorImage);
Q_DECLARE_METATYPE(TyonTalk);
Q_DECLARE_METATYPE(RTController::TProfile);
Q_DECLARE_METATYPE(RTController::TProfiles);
Q_DECLARE_METATYPE(RTController::TColorItem);
Q_DECLARE_METATYPE(RTController::TDeviceColors);
Q_DECLARE_METATYPE(RTController::TPhysicalButton);
Q_DECLARE_METATYPE(RTController::TButtonLink);
