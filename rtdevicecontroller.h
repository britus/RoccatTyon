#pragma once
#include "rthiddevice.h"
#include "rtprofilemodel.h"
#include "rttypes.h"
#include <QKeyCombination>
#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QtCompare>

#ifndef CB_CTLR
#define CB_CTLR(x) RTViewController::x
#endif

#ifndef CB_BIND
#define CB_BIND(o, x) std::bind(x, o, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
#endif

class RTDeviceController : public QAbstractItemModel
{
    Q_OBJECT

public:
    typedef struct
    {
        QString name;
        TyonButtonIndex standard_index;
        TyonButtonIndex easyshift_index;
    } TPhysicalButton;

    typedef std::function<int( //
        TyonButtonIndex,
        TyonButtonType,
        const QKeyCombination &)>
        THandlerSetButton;

    typedef struct
    {
        TyonButtonIndex index;
        THandlerSetButton handler;
    } TButtonLink;

    explicit RTDeviceController(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool clearItemData(const QModelIndex &index) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    /**
     * @brief Find ROCCAT Tyon device
     */
    void lookupDevice();

    /**
     * @brief buttonTypes
     * @return
     */
    inline bool hasDevice() const { return m_hasDevice; }

    /**
     * @brief Assign given mouse button to specific function
     * @param The native mouse button type
     * @param The native function to be assigned
     * @return 0 if success
     */
    int assignButton(TyonButtonIndex, TyonButtonType, const QKeyCombination &);

    /**
     * @brief setupButton
     * @param type
     * @param button
     * @return
     */
    void setupButton(const RoccatButton &rb, QPushButton *button);

    /**
     * @brief toKeySequence
     * @param button
     * @return
     */
    const QKeySequence toKeySequence(const RoccatButton &button) const;

    /**
     * @brief profileIndex
     * @param pix
     * @return QModelIndex
     */
    inline QModelIndex profileIndex(uint pix) { return index(pix, 0, {}); }

    /**
     * @brief buttonTypes
     * @return
     */
    inline const QMap<quint8, QString> &buttonTypes() { return m_buttonTypes; }

    /**
     * @brief selectProfile
     * @param profile
     */
    void selectProfile(quint8 profileIndex);

    /**
     * @brief Convert Roccat sensitivity X value to UI value
     * @param settings
     * @return
     */
    qint16 toSensitivityXValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert Roccat sensitivity Y value to UI value
     * @param settings
     * @return
     */
    qint16 toSensitivityYValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert Roccat DPI level to UI value
     * @param settings
     * @param index
     * @return
     */
    quint16 toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const;

    void setXSensitivity(quint8 sensitivity);
    void setYSensitivity(quint8 sensitivity);
    void setAdvancedSenitivity(quint8 bit, bool state);
    void setPollRate(quint8 rate);
    void setDpiSlot(quint8 bit, bool state);
    void setActiveDpiSlot(quint8 id);
    void setDpiLevel(quint8 index, quint16 value);
    void setLightsEnabled(quint8 flag, bool state);
    void setLightsEffect(quint8 value);
    void setColorFlow(quint8 value);
    void setLightColorWheel(const TyonRmpLightInfo &color);
    void setLightColorBottom(const TyonRmpLightInfo &color);

    QString profileName() const;

    bool loadProfilesFromFile(const QString &fileName);
    bool saveProfilesToFile(const QString &fileName);
    bool saveProfilesToDevice();
    bool resetProfiles();

signals:
    void lookupStarted();
    void deviceFound();
    void deviceRemoved();
    void deviceError(int error, const QString &message);
    void deviceInfoChanged(const TyonInfo &info);
    void profileIndexChanged(quint8 index);
    void settingsChanged(const TyonProfileSettings &settings);
    void buttonsChanged(const TyonProfileButtons &buttons);
    void saveProfilesStarted();
    void saveProfilesFinished();

private slots:
    void onLookupStarted();
    void onDeviceError(int error, const QString &message);
    void onDeviceFound();
    void onDeviceRemoved();
    void onDeviceInfo(const TyonInfo &info);
    void onProfileIndexChanged(const quint8 pix);
    void onProfileChanged(const RTHidDevice::TProfile &profile);
    void onSaveProfilesStarted();
    void onSaveProfilesFinished();

private:
    RTHidDevice m_device;
    QMap<quint8, QString> m_buttonTypes;
    QMap<quint8, RTDeviceController::TPhysicalButton> m_physButtons;
    bool m_hasDevice;

private:
    inline void initButtonTypes();
    inline void initPhysicalButtons();
    inline void setButtonType(const QString &name, quint8 type);
    inline void setPhysicalButton(quint8 index, TPhysicalButton pb);
};

Q_DECLARE_METATYPE(RTDeviceController::TPhysicalButton);
Q_DECLARE_METATYPE(RTDeviceController::TButtonLink);
