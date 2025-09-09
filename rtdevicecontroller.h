#pragma once
#include "rthiddevice.h"
#include "rttypes.h"
#include <QKeyCombination>
#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QtCompare>

#ifndef CB_BIND
#define CB_BIND(o, x) std::bind(x, o, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
#endif

/**
 * @brief The RTDeviceController class is a proxy between device and UI
 */
class RTDeviceController : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
     * Callback function to assign ROCCAT Tyon button function
     */
    typedef std::function<int(TyonButtonIndex, TyonButtonType, const QKeyCombination &)> TSetButtonCallback;

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
     * @brief Default constructor
     * @param parent NULL or QObject
     */
    explicit RTDeviceController(QObject *parent = nullptr);

    /**
     * @brief Find ROCCAT Tyon device
     */
    void lookupDevice();

    /**
     * @brief Check device is present
     * @return true if present
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
     * @brief Link ROCCAT Tyon button descriptor to UI QPushButton
     * @param rb ROCCAT button descriptor
     * @param button UI push button object
     */
    void setupButton(const RoccatButton &rb, QPushButton *button);

    /**
     * @brief Translate ROCCAT Tyon shortcut to QT key sequence
     * @param button ROCCAT button descriptor
     * @return QKeySequence object
     */
    const QKeySequence toKeySequence(const RoccatButton &button) const;

    /**
     * @brief Return the model index of given profile index
     * @param ROCCAT Tyon profile index
     * @return QModelIndex object
     */
    inline QModelIndex profileIndex(uint pix) { return index(pix, 0, {}); }

    /**
     * @brief Return the descriptor map for the UI menu
     * @return QMap object
     */
    inline const QMap<quint8, QString> &buttonTypes() { return m_buttonTypes; }

    /**
     * @brief Set active ROCCAT Tyon profile index
     * @param profileIndex 0 - 4
     */
    void selectProfile(quint8 profileIndex);

    /**
     * @brief Convert ROCCAT Tyon sensitivity X value to UI value
     * @param settings Pointer to ROCCAT Tyon settings structure
     * @return
     */
    qint16 toSensitivityXValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert ROCCAT Tyon sensitivity Y value to UI value
     * @param settings Pointer to ROCCAT Tyon settings structure
     * @return
     */
    qint16 toSensitivityYValue(const TyonProfileSettings *settings) const;

    /**
     * @brief Convert ROCCAT Tyon DPI level to UI value
     * @param settings Pointer to ROCCAT Tyon settings structure
     * @param index DPI level index
     * @return DPI value for the UI
     */
    quint16 toDpiLevelValue(const TyonProfileSettings *settings, quint8 index) const;

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

    /**
     * @brief Set the color for the wheel or bottom light
     * @param target 0=Wheel or 1=Bottom
     * @param light Tyon light info type
     */
    void setLightColor(TyonLightType target, const TyonLight &light);

    /**
     * @brief Return the name of the active profile
     * @return QString
     */
    QString profileName() const;

    /**
     * @brief Load all ROCCAT Tyon profiles from file
     * @param fileName The file name
     * @return true if success
     */
    bool loadProfilesFromFile(const QString &fileName);

    /**
     * @brief Save all ROCCAT Tyon profiles to file
     * @param fileName The file name
     * @return true if success
     */
    bool saveProfilesToFile(const QString &fileName);

    /**
     * @brief Save all modified ROCCAT Tyon profiles to device
     * @return true if success
     */
    bool saveProfilesToDevice();

    /**
     * @brief Reset all ROCCAT Tyon profiles to device defaults
     * @return true if success
     */
    bool resetProfiles();

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
    inline const RTHidDevice::TDeviceColors &deviceColors() const { return m_device.deviceColors(); }

    /* ------------------------------------------------------
     * QAbstractItemModel interface
     * ------------------------------------------------------ */

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

signals:
    void lookupStarted();
    void deviceFound();
    void deviceRemoved();
    void deviceError(int error, const QString &message);
    void deviceInfoChanged(const TyonInfo &info);
    void profileIndexChanged(quint8 index);
    void settingsChanged(const TyonProfileSettings &settings);
    void buttonsChanged(const TyonProfileButtons &buttons);
    void deviceWorkerStarted();
    void deviceWorkerFinished();

private slots:
    void onLookupStarted();
    void onDeviceError(int error, const QString &message);
    void onDeviceFound();
    void onDeviceRemoved();
    void onDeviceInfo(const TyonInfo &info);
    void onProfileIndexChanged(const quint8 pix);
    void onProfileChanged(const RTHidDevice::TProfile &profile);
    void onDeviceWorkerStarted();
    void onDeviceWorkerFinished();

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
