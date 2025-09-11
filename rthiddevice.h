#pragma once
#include "rttypes.h"
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
        quint8 talkFxState;
        TyonProfileSettings settings;
        TyonProfileButtons buttons;
    } TProfile;

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
    inline quint8 profileIndex() const { return m_profile.profile_index; }

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

    quint8 minimumXCelerate() const;
    quint8 maximumXCelerate() const;
    quint8 middleXCelerate() const;

    uint sensorMedianOfImage(TyonSensorImage const *image);

    bool talkFxState() const;
    TyonControlUnitDcu dcuState() const;
    TyonControlUnitTcu tcuState() const;
    uint tcuMedian() const;

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

    //public slots:
    void reportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data);
    void deviceFoundCallback(IOReturn status, IOHIDDeviceRef device);

public slots:
    /**
     * @brief Find ROCCAT Tyon device
     */
    void lookupDevice();

    /**
     * @brief Save ROCCAT Tyon profiles to device
     * @return True if success
     */
    void saveProfilesToDevice();

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
     * @param bit ROCCAT Tyon selection bits
     * @param state True independent / false X/Y same value
     */
    void setAdvancedSenitivity(quint8 bit, bool state);

    /**
     * @brief Set sensor poll rate
     * @param rate
     */
    void setPollRate(quint8 rate);

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
     * @brief Enable ROCCAT Tyon lights
     * @param flag Bit of the light (top / bottom)
     * @param state True enable / False disable
     */
    void setLightsEnabled(quint8 flag, bool state);

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
    void setTcuState(TyonControlUnitTcu state);

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

public:
    void onDeviceFound(IOHIDDeviceRef device);
    void onDeviceRemoved(IOHIDDeviceRef device);
    void onSetReport(IOReturn status, uint rid, CFIndex length, uint8_t *report);
    void onSpecialReport(uint rid, CFIndex length, uint8_t *report);

private:
    IOHIDManagerRef m_manager;
    QList<IOHIDDeviceRef> m_wrkrDevices;
    QList<IOHIDDeviceRef> m_miscDevices;
    TDeviceColors m_colors;
    TyonInfo m_info;
    TyonProfile m_profile;
    TProfiles m_profiles;
    QMutex m_waitMutex;
    QMutex m_accessMutex;
    bool m_isCBComplete;
    bool m_initComplete;
    quint8 m_requestedProfile;
    TyonControlUnit m_controlUnit;
    TyonSensor m_sensor;
    TyonSensorImage m_sensorImage;
    dispatch_queue_t m_hidQueue;

    uint8_t m_cbReportBuffer[4096];
    uint m_cbReportLength = 4096;

private:
    inline void releaseDevices();
    inline void releaseManager();
    inline void initializeProfiles();
    inline void initializeColorMapping();
    inline void internalSaveProfiles();
    inline int raiseError(int error, const QString &message);
    inline void setModified(quint8 pix, bool changed);
    inline void setModified(TProfile *p, bool changed);
    inline void updateProfileMap(TProfile *p, bool changed);
    inline int hidGetReportById(IOHIDDeviceRef device, int reportId, CFIndex size);
    inline int hidWriteRoccatCtl(IOHIDDeviceRef device, uint pix, uint req);
    inline int hidCheckWrite(IOHIDDeviceRef device);
    inline int hidSetReportRaw(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length);
    inline int hidWriteReport(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length);
    inline int hidWriteWithCB(IOHIDDeviceRef device, IOHIDReportType hrt, CFIndex rid, const quint8 *buffer, CFIndex length);
    inline int hidParseResponse(int reportId, const quint8 *buffer, CFIndex length);
    inline int setDeviceState(bool state, IOHIDDeviceRef device = nullptr);
    inline int selectProfileSettings(IOHIDDeviceRef device, uint pix);
    inline int selectProfileButtons(IOHIDDeviceRef device, uint pix);
    inline int selectMacro(IOHIDDeviceRef device, uint pix, uint dix, uint bix);
    inline int readProfile(IOHIDDeviceRef device, quint8 pix);
    inline int readButtonMacro(IOHIDDeviceRef device, uint pix, uint bix);
    inline int readDeviceInfo(IOHIDDeviceRef device);
    inline int readActiveProfile(IOHIDDeviceRef device);
    inline int readDeviceControl(IOHIDDeviceRef device);
    inline int readDeviceSpecial(IOHIDDeviceRef device);
    inline int readProfileSettings(IOHIDDeviceRef device);
    inline int readProfileButtons(IOHIDDeviceRef device);
    inline int sensorRead(IOHIDDeviceRef device);
    inline int sensorReadImage(IOHIDDeviceRef device);
    inline int readDeviceControlUnit(IOHIDDeviceRef device);
    inline int readDeviceTalk(IOHIDDeviceRef device);
    inline int readDevice0A(IOHIDDeviceRef device);
    inline int readDevice11(IOHIDDeviceRef device);
    inline int readDevice1A(IOHIDDeviceRef device);
    //--
    inline int tcuWriteTest(IOHIDDeviceRef device, TyonControlUnitDcu dcuState, uint median);
    inline int tcuWriteAccept(IOHIDDeviceRef device, TyonControlUnitDcu dcuState, uint median);
    inline int tcuWriteCancel(IOHIDDeviceRef device, TyonControlUnitDcu dcuState);
    inline int tcuWriteOff(IOHIDDeviceRef device, TyonControlUnitDcu dcuState);
    inline int tcuWriteTry(IOHIDDeviceRef device, TyonControlUnitDcu dcuState);
    // --
    inline int dcuWriteState(IOHIDDeviceRef device, TyonControlUnitDcu dcuState);
    //--
    inline int sensorWriteStruct(IOHIDDeviceRef device, quint8 action, quint8 reg, quint8 value);
    inline int sensorWriteRegister(IOHIDDeviceRef device, quint8 reg, quint8 value);
    inline int sensorReadRegister(IOHIDDeviceRef device, quint8 reg);
    inline int sensorCaptureImage(IOHIDDeviceRef device);
    //--
    inline int xcCalibWriteStart(IOHIDDeviceRef device);
    inline int xcCalibWriteData(IOHIDDeviceRef device, quint8 min, quint8 mid, quint8 max);
    inline int xcCalibWriteEnd(IOHIDDeviceRef device);
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
Q_DECLARE_METATYPE(RTHidDevice::TProfile);
Q_DECLARE_METATYPE(RTHidDevice::TProfiles);
Q_DECLARE_METATYPE(RTHidDevice::TColorItem);
Q_DECLARE_METATYPE(RTHidDevice::TDeviceColors);
