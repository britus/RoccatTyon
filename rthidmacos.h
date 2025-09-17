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
     * @brief readHidMessage
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool readHidMessage(quint32 reportId, qsizetype length) override;

    /**
     * @brief writeHidMessage
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool writeHidMessage(quint32 reportId, quint8 *buffer, qsizetype length) override;

    /**
     * @brief writeHidAsync
     * @param reportId
     * @param buffer
     * @param length
     * @return
     */
    bool writeHidAsync(quint32 reportId, quint8 *buffer, qsizetype length) override;

public slots:
    /**
     * @brief Find ROCCAT Tyon device
     */
    bool lookupDevices(quint32 vendorId, QList<quint32> products) override;

protected:
    // Callback HIDManager level
    static void _deviceAttachedCallback(void *, IOReturn, void *, IOHIDDeviceRef);
    static void _deviceRemovedCallback(void *, IOReturn, void *, IOHIDDeviceRef);
    // Callback per HID device
    static void _inputCallback(void *, IOReturn, void *, IOHIDReportType, uint32_t, uint8_t *, CFIndex);
    static void _inputValueCallback(void *context, IOReturn result, void *device, IOHIDValueRef value);
    // Callback per HID report (write async)
    static void _reportCallback(void *, IOReturn, void *, IOHIDReportType, uint32_t, uint8_t *, CFIndex);

protected:
    void doDeviceFound(IOHIDDeviceRef device);
    void doDeviceRemoved(IOHIDDeviceRef device);
    void doDeviceInput(quint32 rid, qsizetype length, quint8 *report);
    void doReportSent(IOReturn status, quint32 rid, qsizetype length, quint8 *report);

private:
    IOHIDManagerRef m_manager;
    IOHIDDeviceRef m_ctrlDevice;
    IOHIDDeviceRef m_inputDevice;
    // --
    QMap<quint8, TReportHandler> m_handlers;
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
    inline void hidDeviceProperties(IOHIDDeviceRef device, THidDeviceInfo *info) const;
    inline int hidReportById(IOHIDDeviceRef device, int rid, CFIndex size);
    inline int hidReadReport(IOHIDDeviceRef device, quint8 rid, quint8 *buffer, CFIndex size);
    inline int hidWriteReport(IOHIDDeviceRef device, CFIndex rid, const quint8 *buffer, CFIndex length);
    inline int hidWriteAsync(IOHIDDeviceRef device, const uint8_t *buffer, CFIndex length);
};
