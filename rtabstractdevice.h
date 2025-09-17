#pragma once
#include <QObject>

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

/**
 * @brief HID report response handler function
 */
typedef std::function<int(const quint8 *, qsizetype)> TReportHandler;

/**
 * @brief The HID device interface
 */
class RTHidDevice
{
public:
    virtual ~RTHidDevice() {};
    virtual void registerHandlers(const QMap<quint32, TReportHandler> &handlers) = 0;
    virtual bool lookupDevices(quint32 vendorId, QList<quint32> products) = 0;
    virtual bool readHidMessage(quint32 reportId, qsizetype length) = 0;
    virtual bool writeHidMessage(quint32 reportId, quint8 *buffer, qsizetype length) = 0;
    virtual bool writeHidAsync(quint32 reportId, quint8 *buffer, qsizetype length) = 0;
};

/**
 * @brief Abstract HID device class with signals
 */
class RTAbstractDevice : public QObject, public RTHidDevice
{
    Q_OBJECT
    Q_INTERFACES(RTHidDevice)

public:
    typedef enum {
        MouseControl = 0,
        MouseInput = 1,
    } TDeviceType;
    Q_ENUM(TDeviceType)

    /**
     * @brief Constructor
     * @param parent
     */
    explicit RTAbstractDevice(QObject *parent = nullptr)
        : QObject(parent)
    {}

signals:
    void deviceFound(RTAbstractDevice *device, TDeviceType type);
    void deviceRemoved();
    void lookupStarted();
    void errorOccured(int error, const QString &message);
    void readyRead(quint32 rid, quint8 *buffer, qsizetype length);
};
Q_DECLARE_METATYPE(RTAbstractDevice::TDeviceType)
