// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once
#include "rttypedefs.h"
#include <IOKit/hid/IOHIDManager.h>
#include <dispatch/dispatch.h>
#include <QAbstractItemModel>
#include <QColor>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QWaitCondition>

#define HIDAPI_MAX_STR 255

/**
 * @brief The RTHidDevice class manage the access to the ROCCAT Tyon mouse using the HID protocol
 */
class RTHidDevice : public QObject
{
    Q_OBJECT

public:
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
    explicit RTHidDevice(QObject *parent = nullptr);

    /** */
    ~RTHidDevice();

    /**
     * @brief Return true if devices found
     * @return True or False
     */
    inline bool hasDevice() const { return !m_wrkrDevices.isEmpty(); }

    /**
     * @brief Return active profile index
     * @return Profile index
     */
    inline quint8 profileIndex() const { return m_activeProfile.profile_index; }

    /**
     * @brief Return number of device profiles
     * @return TYON_PROFILE_NUM
     */
    inline quint8 profileCount() const { return m_profiles.count(); }

    /**
     * @brief Return number of device profiles
     * @return TYON_PROFILE_NUM
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
     * @brief Assign ROCCAT Tyon button function
     * @param type Physical button
     * @param func Button function
     * @param QKeyCombination If short function, the shortcut
     */
    void assignButton(TyonButtonIndex type, TyonButtonType func, const QKeyCombination &kc);

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
     * @return
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
     * @return rate value
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
    void profileChanged(const RTHidDevice::TProfile &profile);
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

protected:
    void onDeviceFound(IOHIDDeviceRef device);
    void onDeviceRemoved(IOHIDDeviceRef device);
    void onSetReport(IOReturn status, uint rid, CFIndex length, uint8_t *report);
    void onSpecialReport(uint rid, CFIndex length, uint8_t *report);

protected:
    // called from static HID callback functions
    void doDeviceFoundCallback(IOReturn status, IOHIDDeviceRef device);
    void doReportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data);

protected:
    // Callback HIDManager level
    static void _deviceAttachedCallback(void *context, //
                                        IOReturn,
                                        void *,
                                        IOHIDDeviceRef device);
    static void _deviceRemovedCallback(void *context, //
                                       IOReturn,
                                       void *,
                                       IOHIDDeviceRef device);
    // Callback per HID device
    static void _inputCallback(void *context, //
                               IOReturn result,
                               void *device,
                               IOHIDReportType /*type*/,
                               uint32_t reportID,
                               uint8_t *report,
                               CFIndex reportLength);
    static void _inputValueCallback(void *context, //
                                    IOReturn result,
                                    void *device,
                                    IOHIDValueRef value);
    // Callback per HID report
    static void _reportCallback(void *context, //
                                IOReturn result,
                                void *device,
                                IOHIDReportType type,
                                uint32_t reportID,
                                uint8_t *report,
                                CFIndex reportLength);

private:
    IOHIDManagerRef m_manager;
    QMutex m_waitMutex;
    QMutex m_accessMutex;
    QList<IOHIDDeviceRef> m_wrkrDevices;
    QList<IOHIDDeviceRef> m_miscDevices;
    QMap<quint8, TReportHandler> m_handlers;
    // --
    TDeviceColors m_colors;
    TyonInfo m_info;
    TyonProfile m_activeProfile;
    TProfiles m_profiles;
    TyonTalk m_talkFx;
    TyonSensor m_sensor;
    TyonSensorImage m_sensorImage;
    TyonControlUnit m_controlUnit;
    // --
    quint8 m_requestedProfile;
    bool m_isCBComplete;
    bool m_initComplete;

    // for input report callback
    uint8_t m_inputBuffer[4096];
    uint m_inputLength = 4096;

private:
    inline void initializeProfiles();
    inline void initializeColorMapping();
    inline void initializeHandlers();

    inline void releaseManager();
    inline void releaseDevices();

    inline int raiseError(int error, const QString &message);

    inline void internalSaveProfiles();

    inline void setModified(quint8 pix, bool changed);
    inline void setModified(TProfile *p, bool changed);
    inline void updateProfile(TProfile &p, bool changed);

    inline int roccatControlWrite(IOHIDDeviceRef device, uint pix, uint req);
    inline int roccatControlCheck(IOHIDDeviceRef device);

    inline int hidGetReportById(IOHIDDeviceRef device, int reportId, CFIndex size);
    inline int hidGetReportRaw(IOHIDDeviceRef device, quint8 rid, quint8 *buffer, CFIndex size);
    inline int hidWriteReport(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length);
    inline int hidWriteReportAsync(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length);

    // get state of device
    inline int readDeviceControl(IOHIDDeviceRef device);
    inline int setDeviceState(bool state, IOHIDDeviceRef device = nullptr);

    // get firmware,DFU,X-Celerator info
    inline int readDeviceInfo(IOHIDDeviceRef device);

    // get and set device profiles
    inline int readActiveProfile(IOHIDDeviceRef device);
    inline int readProfiles(IOHIDDeviceRef device, quint8 pix);
    inline int selectProfileSettings(IOHIDDeviceRef device, uint pix);
    inline int readProfileSettings(IOHIDDeviceRef device);
    inline int selectProfileButtons(IOHIDDeviceRef device, uint pix);
    inline int readProfileButtons(IOHIDDeviceRef device);

    // get and set button macros
    inline int selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix);
    inline int readButtonMacro(IOHIDDeviceRef device, uint pix, uint bix);

    // X-Celerator calibration
    inline int xcCalibWriteStart(IOHIDDeviceRef device);
    inline int xcCalibWriteEnd(IOHIDDeviceRef device);
    inline int xcCalibWriteData(IOHIDDeviceRef device, quint8 min, quint8 mid, quint8 max);

    //-
    inline int readTalkFx(IOHIDDeviceRef device);

    //--
    inline int readControlUnit(IOHIDDeviceRef device);
    inline int tcuWriteTest(IOHIDDeviceRef device, quint8 dcuState, uint median);
    inline int tcuWriteAccept(IOHIDDeviceRef device, quint8 dcuState, uint median);
    inline int tcuWriteCancel(IOHIDDeviceRef device, quint8 dcuState);
    inline int tcuWriteOff(IOHIDDeviceRef device, quint8 dcuState);
    inline int tcuWriteTry(IOHIDDeviceRef device, quint8 dcuState);
    inline int tcuReadSensor(IOHIDDeviceRef device);
    inline int tcuReadSensorImage(IOHIDDeviceRef device);
    inline int tcuReadSensorRegister(IOHIDDeviceRef device, quint8 reg);
    inline int tcuWriteSensorCommand(IOHIDDeviceRef device, quint8 action, quint8 reg, quint8 value);
    inline int tcuWriteSensorRegister(IOHIDDeviceRef device, quint8 reg, quint8 value);
    inline int tcuWriteSensorImageCapture(IOHIDDeviceRef device);
    // --
    inline int dcuWriteState(IOHIDDeviceRef device, quint8 dcuState);
    //--
    inline int talkWriteReport(IOHIDDeviceRef device, TyonTalk *talk);
    inline int talkWriteKey(IOHIDDeviceRef device, quint8 easyshift, quint8 easyshift_lock, quint8 easyaim);
    inline int talkWriteEasyshift(IOHIDDeviceRef device, quint8 state);
    inline int talkWriteEasyshiftLock(IOHIDDeviceRef device, quint8 state);
    inline int talkWriteEasyAim(IOHIDDeviceRef device, quint8 state);
    //--
    inline int talkWriteFxData(IOHIDDeviceRef device, TyonTalk *tyonTalk);
    inline int talkWriteFx(IOHIDDeviceRef device, quint32 effect, quint32 ambient_color, quint32 event_color);
    inline int talkWriteFxState(IOHIDDeviceRef device, quint8 state);
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
Q_DECLARE_METATYPE(RTHidDevice::TProfile);
Q_DECLARE_METATYPE(RTHidDevice::TProfiles);
Q_DECLARE_METATYPE(RTHidDevice::TColorItem);
Q_DECLARE_METATYPE(RTHidDevice::TDeviceColors);
