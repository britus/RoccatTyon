#pragma once
#include "rttypes.h"
#include <IOKit/hid/IOHIDManager.h>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QWaitCondition>

class RTHidDevice : public QObject
{
    Q_OBJECT

public:
    typedef struct
    {
        QString name;
        quint8 index;
        TyonProfileSettings settings;
        TyonProfileButtons buttons;
    } TProfile;

    typedef QMap<quint8, TProfile> TProfiles;

    /**
     * @brief RTHidDevice
     * @param parent
     */
    explicit RTHidDevice(QObject *parent = nullptr);

    /**
     */
    ~RTHidDevice();

    /**
     * @brief Find ROCCAT Tyon device
     */
    void lookupDevice();

    /**
     * @brief Return active profile number
     * @return
     */
    inline quint8 profileIndex() const { return m_profile.profile_index; }

    /**
     * @brief Return a 'idx/profile' map
     * @return A list of profiles
     */
    inline const TProfiles &profiles() const { return m_profiles; }

#if 0
    /**
     * @brief Return active profile
     * @return RTHidDevice::TProfile type
     */
    inline const RTHidDevice::TProfile currentProfile() const
    {
        if (profileIndex() < m_profiles.count()) {
            return m_profiles[profileIndex()];
        } else {
            return {};
        }
    }
#endif

    /**
     * @brief profileName
     * @return
     */
    QString profileName() const;

    /**
     * @brief saveProfilesToDevice
     * @return
     */
    bool saveProfilesToDevice();

    /**
     * @brief saveProfilesToFile
     * @param fileName
     * @return
     */
    bool saveProfilesToFile(const QString &fileName);

    /**
     * @brief loadProfilesFromFile
     * @param fileName
     * @return
     */
    bool loadProfilesFromFile(const QString &fileName);

    /**
     * @brief resetProfiles
     * @return
     */
    bool resetProfiles();

    /**
     * @brief assignButton
     * @param type
     * @param func
     * @param key
     * @param mods
     */
    void assignButton(TyonButtonIndex type, TyonButtonType func, quint8 key, quint8 mods);

    void setActiveProfile(quint8 profileIndex);
    void setProfileName(const QString &name, quint8 profileIndex);
    void setXSensitivity(quint8 sensitivity);
    void setYSensitivity(quint8 sensitivity);
    void setAdvancedSenitivity(quint8 bit, bool state);
    void setPollRate(quint8 rate);
    void setDpiSlot(quint8 bit, bool state);
    void setActiveDpiSlot(quint8 id);
    void setDpiLevel(quint8 index, quint8 value);
    void setLightsEnabled(quint8 flag, bool state);
    void setLightsEffect(quint8 value);
    void setColorFlow(quint8 value);
    void setLightColorWheel(quint8 red, quint8 green, quint8 blue);
    void setLightColorBottom(quint8 red, quint8 green, quint8 blue);

signals:
    void lookupStarted();
    void deviceFound();
    void deviceRemoved();
    void deviceError(int error, const QString &message);
    void deviceInfo(const TyonInfo &info);
    void profileIndexChanged(const quint8 pix);
    void profileChanged(const RTHidDevice::TProfile &profile);

    //public slots:
    void reportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data);
    void deviceFoundCallback(IOReturn status, IOHIDDeviceRef device);

public slots:
    void onSetReportCallback(IOReturn status, uint rid, CFIndex length, const QByteArray &data);
    void onDeviceFound(IOHIDDeviceRef device);
    void onDeviceRemoved(IOHIDDeviceRef device);

private:
    IOHIDManagerRef m_manager;
    QList<IOHIDDeviceRef> m_devices;
    TyonInfo m_info;
    TyonProfile m_profile;
    TProfiles m_profiles;
    QMutex m_mutex;
    bool m_isCBComplete;
    bool m_hasHidApi;

private:
    inline void releaseDevices();
    inline void releaseManager();
    inline int hidGetReportById(IOHIDDeviceRef device, int reportId, CFIndex size);
    inline int hidSetRoccatControl(IOHIDDeviceRef device, uint ep, uint rid, uint pix, uint req);
    inline int hidWriteReport(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length);
    inline int hidWriteRaw(IOHIDDeviceRef device, quint8 const *buffer, ssize_t length);
    inline int parsePayload(int reportId, const quint8 *buffer, CFIndex length);
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
    inline int readDeviceSensor(IOHIDDeviceRef device);
    inline int readDeviceControlUnit(IOHIDDeviceRef device);
    inline int readDeviceTalk(IOHIDDeviceRef device);
    inline int readDevice0A(IOHIDDeviceRef device);
    inline int readDevice11(IOHIDDeviceRef device);
    inline int readDevice1A(IOHIDDeviceRef device);
};

Q_DECLARE_METATYPE(RTHidDevice::TProfile);
