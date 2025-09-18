#pragma once
#include <QMap>
#include <QObject>
#include <QDebug>

/**
 * @brief HID device information
 */
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
    uint primaryUsage;     //  MacOS: Mouse = 0x04 or Misc = 0x00
    uint primaryUsagePage; //  MacOS: Mouse = 0x01 or Misc = 0x0a
} THidDeviceInfo;
Q_DECLARE_METATYPE(THidDeviceInfo)

typedef enum {
    HidUnknown = 0,
    HidMouseControl,
    HidMouseInput,
    HidJoystick,
} THidDeviceType;
Q_DECLARE_METATYPE(THidDeviceType)

/**
 * @brief HID report response handler function
 */
typedef std::function<bool(const quint8 *, qsizetype)> TReportHandler;
typedef QMap<quint32, TReportHandler> TReportHandlers;

/**
 * @brief The HID device interface
 */
class RTHidDevice
{
public:
    virtual ~RTHidDevice() {};
    virtual void registerHandlers(const TReportHandlers &handlers) = 0;
    virtual bool lookupDevices(quint32 vendorId, QList<quint32> products) = 0;
    virtual bool openDevice(THidDeviceType type) = 0;
    virtual bool closeDevice(THidDeviceType type) = 0;
    virtual bool readHidMessage(THidDeviceType type, quint32 reportId, qsizetype length) = 0;
    virtual bool readHidMessage(THidDeviceType type, quint32 reportId, quint8 *buffer, qsizetype length) = 0;
    virtual bool writeHidMessage(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length) = 0;
    virtual bool writeHidAsync(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length) = 0;
    virtual bool hasDevice() const = 0;
};
Q_DECLARE_INTERFACE(RTHidDevice, "org.eof.tools.RoccatTyon.RTHidDevice")

/**
 * @brief Abstract HID device class with signals
 */
class RTAbstractDevice : public QObject, public RTHidDevice
{
    Q_OBJECT
    Q_INTERFACES(RTHidDevice)

public:
    /**
     * @brief Constructor
     * @param parent
     */
    explicit RTAbstractDevice(QObject *parent = nullptr)
        : QObject(parent)
    {}

protected:
    virtual int raiseError(int error, const QString &message) {
        qCritical("[HIDDEV] Error 0x%08x: %s", error, qPrintable(message));
        emit errorOccured(error, message);
        return error;
    }

signals:
    void deviceFound(THidDeviceType type);
    void deviceRemoved();
    void lookupStarted();
    void errorOccured(int error, const QString &message);
    void inputReady(quint32 rid, const QByteArray &data);
};
