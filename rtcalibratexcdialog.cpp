#include "rtcalibratexcdialog.h"
#include "rttypes.h"
#include "ui_rtcalibratexcdialog.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QThread>

enum {
    PHASE_MID = 0,
    PHASE_WAIT_CHANGE_MAX,
    PHASE_WAIT_TIME_MAX,
    PHASE_MAX,
    PHASE_WAIT_CHANGE_MIN,
    PHASE_WAIT_TIME_MIN,
    PHASE_MIN,
    PHASE_END,
};

static const uint next_phases[] = {
    PHASE_WAIT_CHANGE_MAX,
    PHASE_WAIT_TIME_MAX,
    PHASE_MAX,
    PHASE_WAIT_CHANGE_MIN,
    PHASE_WAIT_TIME_MIN,
    PHASE_MIN,
    PHASE_END,
    PHASE_END,
};

#define AVERAGE_SHIFT 7;
static uint const validCount = 1 << AVERAGE_SHIFT;

RTCalibrateXCDialog::RTCalibrateXCDialog(RTDeviceController *controller, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RTCalibrateXCDialog)
    , m_ctlr(controller)
    , m_instructions()
    , m_isSaved(false)
    , m_stage(0)
    , m_count(0)
    , m_phaseStart()
    , m_last_value(0)
    , m_min(255)
    , m_max(0)
    , m_mid(0)
{
    ui->setupUi(this);
    ui->swWizzard->setCurrentIndex(0);
    ui->pbApply->setVisible(false);
    ui->pbCancel->setDefault(false);
    ui->pbNextPage->setDefault(true);
    ui->pbNextPage->setFocus();

    //| Qt::WindowStaysOnTopHint| Qt::FramelessWindowHint| Qt::CustomizeWindowHint
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(Qt::Tool);

    m_instructions.append(tr("Please don't touch the paddle for center calibration."));
    m_instructions.append(tr("Please push the paddle all the way up and hold it until the next step is displayed."));
    m_instructions.append(tr("Please push the paddle all the way downwards and hold it until the next step is displayed."));
    m_instructions.append(tr("Calibration completed.\n\nClick button 'Apply' to save the calibration or button 'Cancel' to close without saving."));

    connect(m_ctlr, &RTDeviceController::deviceError, this, &RTCalibrateXCDialog::onDeviceError, Qt::DirectConnection);
    connect(m_ctlr, &RTDeviceController::specialReport, this, &RTCalibrateXCDialog::onSpecialReport, Qt::DirectConnection);

    connect(ui->pbNextPage, &QPushButton::clicked, this, [this, parent]() { //
        ui->swWizzard->setCurrentIndex(1);
        ui->pbNextPage->setVisible(false);
        m_ctlr->xcStartCalibration();
        setParentEnabled(parent, false);
    });

    connect(ui->pbApply, &QPushButton::clicked, this, [this, parent]() { //
        const QString title = tr("Save calibration");
        const QString msg = tr("Do you want to apply the calibration to the device?\n");
        ui->pbApply->setVisible(false);
        if (QMessageBox::question(this, title, msg) == QMessageBox::Yes) {
            ui->txInstruction->setText(tr("Well done! Click 'Close' button."));
            m_ctlr->xcApplyCalibration(m_min, m_mid, m_max);
            m_isSaved = true;
        } else {
            ui->txInstruction->setText(tr("X-Celerator calibration not saved! Click 'Close' button."));
            m_isSaved = false;
        }
        setParentEnabled(parent, true);
        ui->pbCancel->setText(tr("Close"));
        ui->pbCancel->setDefault(true);
        m_ctlr->xcStopCalibration();
    });

    connect(ui->pbCancel, &QPushButton::clicked, this, [this, parent]() { //
        m_ctlr->xcStopCalibration();
        setParentEnabled(parent, true);
        if (!m_isSaved) {
            reject();
        } else {
            accept();
        }
    });
}

RTCalibrateXCDialog::~RTCalibrateXCDialog()
{
    m_ctlr->disconnect(this);
    delete ui;
}

void RTCalibrateXCDialog::onDeviceError(int error, const QString &message)
{
    ui->txInstruction->setText(tr("ERROR %1: %2").arg(error, 8, 16, QChar('0')).arg(message));
}

void RTCalibrateXCDialog::onSpecialReport(uint, const QByteArray &report)
{
    TyonSpecial *p = (TyonSpecial *) report.constData();

    // sane check
    if ((ulong) report.length() < sizeof(TyonSpecial)) {
        return;
    }

    int value = p->action;

    switch (m_stage) {
        case PHASE_MID: {
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_MID m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            if (inRange(value, m_last_value, 10)) {
                m_mid += value;
                ++m_count;
                ui->vslXCelerate->setValue(value);
                if (m_count == validCount) {
                    m_mid = m_mid >> AVERAGE_SHIFT;
                    nextPhase();
                }
            } else {
                m_mid = 0;
                m_count = 0;
            }
            break;
        }
        case PHASE_MAX: {
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_MAX m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            if (inRange(value, m_last_value, 10)) {
                m_max += value;
                ++m_count;
                if (value > ui->vslXCelerate->maximum()) {
                    ui->vslXCelerate->setMaximum(value);
                    ui->vslXCelerate->setValue(m_max);
                }
                if (m_count == validCount) {
                    m_max = m_max >> AVERAGE_SHIFT;
                    nextPhase();
                }
            } else {
                m_max = 0;
                m_count = 0;
            }
            break;
        }
        case PHASE_MIN: {
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_MIN m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            if (inRange(value, m_last_value, 10)) {
                m_min += value;
                ++m_count;
                if (value < ui->vslXCelerate->minimum()) {
                    ui->vslXCelerate->setMinimum(value);
                    ui->vslXCelerate->setValue(value);
                }
                if (m_count == validCount) {
                    m_min = m_min >> AVERAGE_SHIFT;
                    nextPhase();
                }
            } else {
                m_min = 0;
                m_count = 0;
            }
            break;
        }
        case PHASE_WAIT_TIME_MAX:
        case PHASE_WAIT_TIME_MIN: {
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_WAIT_TIME_MINMAX m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            /* 1 sec grace period */
            int diff = m_phaseStart.msecsTo(QTime::currentTime()) / 1000;
            if (diff >= 1.0) {
                nextPhase();
            }
            break;
        }
        case PHASE_WAIT_CHANGE_MAX: { /* values < mid */
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_WAIT_CHANGE_MAX m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            if ((value + 20) < m_mid)
                nextPhase();
            break;
        }
        case PHASE_WAIT_CHANGE_MIN: { /* values > mid */
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_WAIT_CHANGE_MIN m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            if (value > (m_mid + 20))
                nextPhase();
            break;
        }
        case PHASE_END:
#ifdef QT_DEBUG
            qDebug("[XC-CAL] PHASE_END m_min=%d m_mid=%d m_max=%d value=%d", m_min, m_mid, m_max, value);
#endif
            ui->vslXCelerate->setVisible(false);
            ui->txInstruction->setText(m_instructions[3]);
            ui->pbNextPage->setVisible(false);
            ui->pbApply->setVisible(true);
            m_stage += 1;
            break;
        default:
            break;
    }
}

inline void RTCalibrateXCDialog::setParentEnabled(QWidget *parent, bool enable)
{
    QMainWindow *mw;
    if ((mw = dynamic_cast<QMainWindow *>(parent))) {
        mw->centralWidget()->setEnabled(enable);
        return;
    }
    QWidget *w;
    if ((w = dynamic_cast<QWidget *>(parent))) {
        w->setEnabled(enable);
        return;
    }
}

inline bool RTCalibrateXCDialog::inRange(qint32 a, qint32 b, qint32 range)
{
    return ((a - b) < range || (b - a) < range) ? TRUE : FALSE;
}

static const uint phaseIndices[] = {0, 1, 1, 1, 2, 2, 2, 3};
inline void RTCalibrateXCDialog::setPhase(uint phase)
{
    m_count = 0;
    m_stage = phase;
    m_phaseStart = QTime::currentTime();
    ui->txInstruction->setText(m_instructions[phaseIndices[phase]]);
}

inline void RTCalibrateXCDialog::nextPhase()
{
    setPhase(next_phases[m_stage]);
}
