#include <QtCore/QtGlobal>

#ifdef Q_OS_LINUX
#include "rthidlinux.h"
#include "rttypedefs.h"
#include <QThread>
#include <QMutexLocker>
#include <linux/ioctl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

//#undef QT_DEBUG

#ifdef QT_DEBUG
static inline void debugReport(const char *where, quint32 rid, const quint8 *buffer, qsizetype length)
{
    const QByteArray d((char *) buffer, length);
    qDebug("[HIDDEV] %s: [RID:0x%02x LEN:%4lld] pl=%s", where, rid, length, qPrintable(d.toHex(' ')));
}
#endif

RTHidLinux::RTHidLinux(QObject *parent)
    : RTAbstractDevice(parent)
    , m_devices()
    , m_handlers()
    , m_monitor(nullptr)
    , m_mutex()
    , m_timer(this)
{
    hid_init();

    m_timer.setInterval(2000);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this](){ //
        if (m_devices.isEmpty()) {
            QList<quint32> products;
            products.append(USB_DEVICE_ID_ROCCAT_TYON_BLACK);
            products.append(USB_DEVICE_ID_ROCCAT_TYON_WHITE);
            lookupDevices(USB_DEVICE_ID_VENDOR_ROCCAT, products);
        }
    });
}

RTHidLinux::~RTHidLinux()
{
    releaseDevices();
    hid_exit();
}

inline void RTHidLinux::releaseDevices()
{
    if (m_monitor) {
        m_monitor->requestInterruption();
        m_monitor->wait();
        m_monitor = 0L;
    }
}

void RTHidLinux::registerHandlers(const TReportHandlers &handlers)
{
    m_handlers = handlers;
}

bool RTHidLinux::hasDevice() const
{
    return !m_devices.isEmpty();
}

static const uint kHIDUsageMouse = 0x01;
static const uint kHIDPageMouse = 0x01;

static const uint kHIDUsageMisc = 0x00;
static const uint kHIDPageMisc = 0x0a;

bool RTHidLinux::lookupDevices(quint32 vendorId, QList<quint32> products)
{
    hid_device_info* devices;

    emit lookupStarted();

    // cleanup prior findings
    releaseDevices();

    // find ROCCAT Tyon device
    foreach(quint32 product, products) {
        if (!(devices = hid_enumerate(vendorId, product))) {
            continue;
        }

        hid_device_info *info = devices;
        while (info) {
            qDebug("Device found --------------------------------");
            qDebug("Device path..: %s", info->path);
            qDebug("Manufacturer.: %s", qPrintable(QString::fromStdWString(info->manufacturer_string)));
            qDebug("Product......: %s", qPrintable(QString::fromStdWString(info->product_string)));
            qDebug("Serialnumber.: %s", qPrintable(QString::fromStdWString(info->serial_number)));
            qDebug("Releasenumber: %d", info->release_number);
            qDebug("Interface....: 0x%02x", info->interface_number);
            qDebug("Primary usage: 0x%02x", info->usage);
            qDebug("Usage page...: 0x%02x", info->usage_page);

            /* Control interface */
            if (info->usage == kHIDUsageMouse && info->usage_page == kHIDPageMouse) {
                THidDevice d = {};
                d.path = QString::fromLatin1(info->path);
                d.interface = info->interface_number;
                m_devices[HidMouseControl] = d;
            }
            /* X-Celerator input interface */
            else if (info->usage == kHIDUsageMisc && info->usage_page == kHIDPageMisc) {
                THidDevice d = {};
                d.path = QString::fromLatin1(info->path);
                d.interface = info->interface_number;
                m_devices[HidMouseInput] = d;
                hidMonitor(d);
            }

            // next entry
            info = info->next;
        }

        // cleanup
        hid_free_enumeration(devices);
    }

    // notify if control interface found
    foreach(auto key, m_devices.keys()) {
        if (key == HidMouseControl) {
            if (m_timer.isActive()) {
                m_timer.stop();
            }
            emit deviceFound(key);
            return true;
        }
    }

    m_timer.start();
    return false;
}

inline THidDevice RTHidLinux::toDevice(THidDeviceType type) const
{
    if (!m_devices.contains(type)) {
        return {};
    }
    return m_devices[type];
}

bool RTHidLinux::openDevice(THidDeviceType type)
{
    Q_UNUSED(type)
    return true;
}

bool RTHidLinux::closeDevice(THidDeviceType type)
{
    Q_UNUSED(type)
    return true;
}

inline void RTHidLinux::hidMonitor(const THidDevice& device)
{
    const Qt::ConnectionType ct = Qt::DirectConnection;

    m_monitor = new RTHidMonitor(device);
    connect(m_monitor, &RTHidMonitor::errorOccured, this, [this](int error, const QString &message) { //
        raiseError(error, message);
    }, ct);
    connect(m_monitor, &RTHidMonitor::inputReady, this, [this](quint32 rid, const QByteArray &data) { //
        emit inputReady(rid, data);
    }, ct);
    connect(m_monitor, &RTHidMonitor::finished, this, [this]() { //
        m_monitor->deleteLater();
        m_monitor = nullptr;
    }, ct);
    connect(m_monitor, &RTHidMonitor::destroyed, this, [device](QObject*) { //
        if (device.dev) {
            hid_close(device.dev);
        }
    }, ct);

    m_monitor->start(QThread::IdlePriority);
}

inline int RTHidLinux::hidReadRaw(THidDeviceType type, qsizetype length, quint8* buffer)
{
    QMutexLocker lock(&m_mutex);

    const THidDevice d = toDevice(type);
    int retval;
    int fd;

    fd = ::open(d.path.toLatin1().constData(), O_RDWR);
    if (fd == -1) {
        return ENODEV;
    }

    retval = ioctl(fd, HIDIOCGFEATURE(length), buffer);
    if (retval == -1) {
        retval = EIO;
    } else {
        retval = 0;
    }

#ifdef QT_DEBUG
    if (buffer[0] && buffer[1] > 0) {
        debugReport("hidReadRaw ", buffer[0], buffer, length);
    }
#endif

    ::close(fd);
    return retval;
}

inline int RTHidLinux::hidWriteRaw(THidDeviceType type, qsizetype length, const quint8* buffer)
{
    QMutexLocker lock(&m_mutex);

    const THidDevice d = toDevice(type);
    int retval;
    int fd;

#ifdef QT_DEBUG
    debugReport("hidWriteRaw", buffer[0], buffer, length);
#endif

    fd = ::open(d.path.toLatin1().constData(), O_RDWR);
    if (fd == -1) {
        return ENODEV;
    }

    retval = ioctl(fd, HIDIOCSFEATURE(length), buffer);
    if (retval == -1) {
        retval = EIO;
    } else {
        retval = 0;
    }

    ::close(fd);
    return retval;
}

bool RTHidLinux::readHidMessage(THidDeviceType type, quint32 rid, qsizetype length)
{
    quint8* buffer = (quint8*) malloc(length);
    int ret;
    // set report identifier
    buffer[0] = rid;
    if ((ret = hidReadRaw(type, length, buffer)) != 0) {
        raiseError(ret, tr("readHidMessage: Error RID=0x%1").arg(rid, 2, 16, QChar('0')));
        free(buffer);
        return false;
    }
    if (m_handlers.contains(rid)) {
        m_handlers[rid](buffer, length);
    }
    free(buffer);
    return true;
}

bool RTHidLinux::readHidMessage(THidDeviceType type, quint32 rid, quint8 *buffer, qsizetype length)
{
    int ret;
    if ((ret = hidReadRaw(type, length, buffer)) != 0) {
        raiseError(ret, tr("readHidMessage: Error RID=0x%1").arg(rid, 2, 16, QChar('0')));
        return false;
    }
    return true;
}

bool RTHidLinux::writeHidMessage(THidDeviceType type, quint32 rid, const quint8 *buffer, qsizetype length)
{
    int ret;
    if ((ret = hidWriteRaw(type, length, buffer)) != 0) {
        raiseError(ret, tr("writeHidMessage: Error RID=0x%1").arg(rid, 2, 16, QChar('0')));
        return false;
    }
    return true;
}

bool RTHidLinux::writeHidAsync(THidDeviceType type, quint32 rid, const quint8 *buffer, qsizetype length)
{
    int ret;
    if ((ret = hidWriteRaw(type, length, buffer)) != 0) {
        raiseError(ret, tr("writeHidAsync: Error RID=0x%1").arg(rid, 2, 16, QChar('0')));
        return false;
    }
    return true;
}

RTHidMonitor::RTHidMonitor(const THidDevice& device)
    : QThread(0L)
    , m_device(device)
{
    //--
}

void RTHidMonitor::run()
{
    const qsizetype length = 64;
    quint8 buffer[length] = {};
    int ret, fd;

    fd = ::open(m_device.path.toLatin1().constData(), O_RDONLY);
    if (fd < 0) {
        emit errorOccured(ENODEV, "Unable to open HID input interface.");
        this->exit(ENODEV);
        return;
    }

    fd_set fdSet, readSet;
    FD_ZERO(&fdSet);
    FD_SET(fd, &fdSet);

    // for select() timeout
    struct timeval tv = {};
    tv.tv_usec = 10000;

    while(!isInterruptionRequested()) {
        readSet = fdSet;
        ret = ::select(fd + 1, &readSet, NULL, NULL, &tv);
        if (ret < 0) { // Error handling
            emit errorOccured(EIO, "Unable to monitor HID input interface.");
            break;
        } else if (ret == 0) { // Timeout, loop again
            this->yieldCurrentThread();
            continue;
        } else {
            ret = ::read(fd, buffer, length);
            if (ret < 0) {
                emit errorOccured(EIO, "Unable to read HID input.");
                break;
            }
            else if(ret && buffer[0]) { // report Id must exist
#ifdef QT_DEBUG
                debugReport("RTHidMonitor", buffer[0], buffer, ret);
#endif
                const QByteArray data((char*)buffer, ret);
                emit inputReady(buffer[0], data);
            }
        }
    }

    ::close(fd);
    this->exit(0);
}

#endif // Q_OS_LINUX
