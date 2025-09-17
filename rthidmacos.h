#pragma once

#include "rtabstractdevice.h"
#include <IOKit/hid/IOHIDManager.h>
#include <dispatch/dispatch.h>
#include <QMap>
#include <QMutex>
#include <QObject>

class RTHidMacOS : public RTAbstractDevice
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent
     */
    explicit RTHidMacOS(QObject *parent = nullptr);

    /**
     *
     */
    ~RTHidMacOS();

    /**
     * @brief Register HID report handlers
     * @param handlers A map of reportId / handler function
     */
    void registerHandlers(const QMap<quint32, TReportHandler> &handlers) override;

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

protected:
    // Callback HIDManager level
    static void _deviceAttached(void *, IOReturn, void *, IOHIDDeviceRef);
    static void _deviceRemoved(void *, IOReturn, void *, IOHIDDeviceRef);
    // Callback per HID device
    static void _deviceInput(void *, IOReturn, void *, IOHIDReportType, uint32_t, uint8_t *, CFIndex);
    static void _inputValueCallback(void *context, IOReturn result, void *device, IOHIDValueRef value);
    // Callback per HID report (write async)
    static void _reportSent(void *, IOReturn, void *, IOHIDReportType, uint32_t, uint8_t *, CFIndex);

protected:
    void doDeviceFound(IOHIDDeviceRef device);
    void doDeviceRemoved(IOHIDDeviceRef device);
    void doDeviceInput(quint32 rid, qsizetype length, quint8 *report);
    void doReportSent(IOReturn status, quint32 rid, qsizetype length, quint8 *report);

private:
    IOHIDManagerRef m_manager;
    IOHIDDeviceRef m_ctrlDevice;
    IOHIDDeviceRef m_miscDevice;
    // --
    TReportHandlers m_handlers;
    // --
    QMutex m_waitMutex;
    bool m_isCBComplete;
    // for input report callback
    uint8_t m_inputBuffer[4096];
    uint m_inputLength = 4096;

private:
    inline void releaseManager();
    inline void releaseDevices();
    // --
    inline int raiseError(int error, const QString &message);
    // --
    inline IOHIDDeviceRef toDevice(THidDeviceType type);
    inline void hidDeviceProperties(IOHIDDeviceRef device, THidDeviceInfo *info) const;
    inline int hidReadAsync(IOHIDDeviceRef device, CFIndex rid, CFIndex size);
    inline int hidReadReport(IOHIDDeviceRef device, CFIndex rid, quint8 *buffer, CFIndex size);
    inline int hidWriteReport(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length);
    inline int hidWriteAsync(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length);
};
