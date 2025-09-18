#ifndef RTHIDLINUX_H
#define RTHIDLINUX_H

#include "rtabstractdevice.h"
#include <hidapi/hidapi.h>
#include <hidapi/hidapi_libusb.h>
#include <QObject>
#include <QMap>
#include <QMutex>
#include <QThread>

typedef struct {
    QString path;
    uint interface;
    hid_device* dev;
} THidDevice;

class RTHidMonitor;
class RTHidLinux : public RTAbstractDevice
{
    Q_OBJECT

public:
    explicit RTHidLinux(QObject *parent = nullptr);

    /**
     *
     */
    ~RTHidLinux();

    /**
     * @brief Register HID report handlers
     * @param handlers A map of reportId / handler function
     */
    void registerHandlers(const TReportHandlers &handlers) override;

    /**
     * @brief hasDevice
     * @return
     */
    bool hasDevice() const override;

    /**
     * @brief openDevice
     * @param type
     * @return
     */
    bool openDevice(THidDeviceType type) override;

    /**
     * @brief closeDevice
     * @param type
     * @return
     */
    bool closeDevice(THidDeviceType type) override;

    /**
     * @brief readHidMessage
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool readHidMessage(THidDeviceType type, quint32 reportId, qsizetype length) override;

    /**
     * @brief readHidMessage
     * @param type
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool readHidMessage(THidDeviceType type, quint32 reportId, quint8 *buffer, qsizetype length) override;
    /**
     * @brief writeHidMessage
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool writeHidMessage(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length) override;

    /**
     * @brief writeHidAsync
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool writeHidAsync(THidDeviceType type, quint32 reportId, const quint8 *buffer, qsizetype length) override;

public slots:
    /**
     * @brief Find ROCCAT Tyon device
     */
    bool lookupDevices(quint32 vendorId, QList<quint32> products) override;

private:
    friend class RTHidMonitor;
    QMap<THidDeviceType, THidDevice> m_devices;
    TReportHandlers m_handlers;
    RTHidMonitor* m_monitor;
    QMutex m_mutex;

private:
    inline void releaseDevices();
    inline THidDevice toDevice(THidDeviceType type) const;
    inline void hidMonitor(const THidDevice& device);
    inline int hidReadRaw(THidDeviceType type, qsizetype length, quint8* buffer);
    inline int hidWriteRaw(THidDeviceType type, qsizetype length, const quint8* buffer);
};

/**
 * @brief This thread monitors the HID input interface
 */
class RTHidMonitor: public QThread
{
    Q_OBJECT

public:
    explicit RTHidMonitor(const THidDevice& device);
    void run() override;

signals:
    void errorOccured(int error, const QString &message);
    void inputReady(quint32 rid, const QByteArray &data);

private:
    THidDevice m_device;
};

#endif // RTHIDLINUX_H
