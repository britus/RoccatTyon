#include "rtmainwindow.h"
#include "rtcolordialog.h"
#include "rtdevicecontroller.h"
#include "rtprogress.h"
#include "rtshortcutdialog.h"
#include "rttypes.h"
#include "ui_rtmainwindow.h"
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QModelIndexList>
#include <QProcessEnvironment>
#include <QRadioButton>
#include <QScreen>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableView>
#include <QTableWidgetItem>
#include <QThread>
#include <QTimer>

RTMainWindow::RTMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::RTMainWindow)
    , m_ctlr(new RTDeviceController(this))
    , m_buttons()
    , m_settings(nullptr)
    , m_rtpfFileName(QStringLiteral("Tyon-Profiles.rtpf"))
{
    qRegisterMetaType<TyonLight>();

    // --
    ui->setupUi(this);

#ifndef QT_DEBUG
    ui->rbLightCustomColor->setEnabled(false);
#endif

    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
    QScreen *scn = qApp->primaryScreen();
    QRect r = scn->availableGeometry();

    //bool isXWindow;
    //isXWindow |= (pe.contains("DISPLAY") || pe.contains("XDG_SESSION_DESKTOP"));

    QStringList sl;
    sl << tr("res:%3x%4").arg(r.size().width()).arg(r.size().height());
    sl << tr("dpi:%1x%2").arg((uint) scn->logicalDotsPerInchX()).arg((uint) scn->logicalDotsPerInchY());
    qInfo() << "[APPWIN] Screen" << sl;

    setWindowTitle(QApplication::applicationDisplayName());

    initializeSettings();
    initializeUiElements();
    loadSettings(m_settings);
    connectController();
    connectUiElements();
    connectActions();

    m_ctlr->lookupDevice();
}

RTMainWindow::~RTMainWindow()
{
    saveSettings(m_settings);
    delete ui;
}

inline void RTMainWindow::initializeSettings()
{
    QString fpath = QStandardPaths::writableLocation( //
        QStandardPaths::AppConfigLocation);

    QDir d(fpath);
    if (!d.exists()) {
        d.mkpath(fpath);
    }

    fpath = QDir::toNativeSeparators(fpath + "/settings.conf");
    m_settings = new QSettings(fpath, QSettings::Format::NativeFormat, this);
}

inline void RTMainWindow::loadSettings(QSettings *settings)
{
    if (!settings)
        return;
    uint value;
    m_settings->beginGroup("ui");

    m_rtpfFileName = m_settings->value("exportName", m_rtpfFileName).toString();

    value = m_settings->value("tabWidget").toUInt();
    ui->tabWidget->setCurrentIndex(value);

    value = m_settings->value("btnWidget").toUInt();
    ui->tbxMButtons->setCurrentIndex(value);

    QVariant v;
    v = m_settings->value("window");
    if (!v.isNull() && v.isValid()) {
        setGeometry(v.toRect());
    }

    m_settings->endGroup();
}

inline void RTMainWindow::saveSettings(QSettings *settings)
{
    if (!settings)
        return;
    m_settings->beginGroup("ui");
    m_settings->setValue("exportName", m_rtpfFileName);
    m_settings->setValue("window", this->geometry());
    m_settings->setValue("tabWidget", ui->tabWidget->currentIndex());
    m_settings->setValue("btnWidget", ui->tbxMButtons->currentIndex());
    m_settings->endGroup();
    m_settings->sync();
}

inline void RTMainWindow::initializeUiElements()
{
    const Qt::FindChildOption fco = Qt::FindChildrenRecursively;

    ui->cbxDPIActiveSlot->setMaxVisibleItems(15);
    ui->tableView->setModel(m_ctlr);
    ui->pnlLeft->setEnabled(false);
    ui->tabWidget->setEnabled(false);
    ui->pbSave->setEnabled(false);
    ui->pbReset->setEnabled(false);

    QSlider *s;
    foreach (QSpinBox *c, ui->pnlSensorDpi->findChildren<QSpinBox *>(fco)) {
        c->setMinimum(TYON_CPI_MIN);
        c->setMaximum(TYON_CPI_MAX);
        c->setSingleStep(TYON_CPI_STEP);
        QString name = QString(c->objectName()).replace("ed", "hs");
        if ((s = ui->pnlSensorDpi->findChild<QSlider *>(name, fco))) {
            c->setProperty("link", QVariant::fromValue(s));
        }
    }

    QSpinBox *b;
    foreach (QSlider *c, ui->pnlSensorDpi->findChildren<QSlider *>(fco)) {
        c->setMinimum(TYON_CPI_MIN);
        c->setMaximum(TYON_CPI_MAX);
        c->setSingleStep(TYON_CPI_STEP);
        QString name = QString(c->objectName()).replace("hs", "ed");
        if ((b = ui->pnlSensorDpi->findChild<QSpinBox *>(name, fco))) {
            c->setProperty("link", QVariant::fromValue(b));
        }
    }

    // Standard
    m_buttons[ui->pbMBStdTopLeft] = {TYON_BUTTON_INDEX_LEFT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopRight] = {TYON_BUTTON_INDEX_RIGHT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopWScrollUp] = {TYON_BUTTON_INDEX_WHEEL_UP, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopWScrollDown] = {TYON_BUTTON_INDEX_WHEEL_DOWN, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopWClick] = {TYON_BUTTON_INDEX_MIDDLE, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopPushLeft] = {TYON_BUTTON_INDEX_FIN_LEFT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopPushRight] = {TYON_BUTTON_INDEX_FIN_RIGHT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopAction2] = {TYON_BUTTON_INDEX_LEFT_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopAction1] = {TYON_BUTTON_INDEX_LEFT_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopAction4] = {TYON_BUTTON_INDEX_RIGHT_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdTopAction3] = {TYON_BUTTON_INDEX_RIGHT_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};

    m_buttons[ui->pbMBStdSideForward] = {TYON_BUTTON_INDEX_THUMB_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdSideBackward] = {TYON_BUTTON_INDEX_THUMB_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdSidePushUp] = {TYON_BUTTON_INDEX_THUMB_PADDLE_UP, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdSidePushDown] = {TYON_BUTTON_INDEX_THUMB_PADDLE_DOWN, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBStdSideEasyShfit] = {TYON_BUTTON_INDEX_THUMB_PEDAL, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};

    // EasyShift //
    m_buttons[ui->pbMBESTopLeft] = {TYON_BUTTON_INDEX_SHIFT_LEFT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopRight] = {TYON_BUTTON_INDEX_SHIFT_RIGHT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopWScrollUp] = {TYON_BUTTON_INDEX_SHIFT_WHEEL_UP, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopWScrollDown] = {TYON_BUTTON_INDEX_SHIFT_WHEEL_DOWN, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopWClick] = {TYON_BUTTON_INDEX_SHIFT_MIDDLE, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopPushLeft] = {TYON_BUTTON_INDEX_SHIFT_FIN_LEFT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopPushRight] = {TYON_BUTTON_INDEX_SHIFT_FIN_RIGHT, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopAction2] = {TYON_BUTTON_INDEX_SHIFT_LEFT_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopAction1] = {TYON_BUTTON_INDEX_SHIFT_LEFT_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopAction4] = {TYON_BUTTON_INDEX_SHIFT_RIGHT_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESTopAction3] = {TYON_BUTTON_INDEX_SHIFT_RIGHT_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};

    m_buttons[ui->pbMBESSideForward] = {TYON_BUTTON_INDEX_SHIFT_THUMB_FORWARD, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESSideBackward] = {TYON_BUTTON_INDEX_SHIFT_THUMB_BACK, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESSidePushUp] = {TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_UP, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESSidePushDown] = {TYON_BUTTON_INDEX_SHIFT_THUMB_PADDLE_DOWN, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};
    m_buttons[ui->pbMBESSideEasyShift_na] = {TYON_BUTTON_INDEX_SHIFT_THUMB_PEDAL, CB_BIND(m_ctlr, &RTDeviceController::assignButton)};

    QActionGroup *agMBBasics = new QActionGroup(this);
    agMBBasics->addAction(linkAction(ui->maAssignMBLeftClick, TYON_BUTTON_TYPE_CLICK));
    agMBBasics->addAction(linkAction(ui->maAssignMBRightCick, TYON_BUTTON_TYPE_MENU));
    agMBBasics->addAction(linkAction(ui->maAssignMBUniScrolling, TYON_BUTTON_TYPE_UNIVERSAL_SCROLLING));
    agMBBasics->addAction(linkAction(ui->maAssignScrollUp, TYON_BUTTON_TYPE_SCROLL_UP));
    agMBBasics->addAction(linkAction(ui->maAssignScrollDown, TYON_BUTTON_TYPE_SCROLL_DOWN));
    agMBBasics->addAction(linkAction(ui->maAssignDoubleClick, TYON_BUTTON_TYPE_DOUBLE_CLICK));
    agMBBasics->addAction(linkAction(ui->maAssignNavForward, TYON_BUTTON_TYPE_BROWSER_FORWARD));
    agMBBasics->addAction(linkAction(ui->maAssignNavBackward, TYON_BUTTON_TYPE_BROWSER_BACKWARD));
    agMBBasics->addAction(linkAction(ui->maAssignTiltLeft, TYON_BUTTON_TYPE_TILT_LEFT));
    agMBBasics->addAction(linkAction(ui->maAssignTiltRight, TYON_BUTTON_TYPE_TILT_RIGHT));
    agMBBasics->addAction(linkAction(ui->maAssignReset, TYON_BUTTON_TYPE_UNUSED));
    m_actions[tr("1 General")] = agMBBasics;

    QActionGroup *agShortcut = new QActionGroup(this);
    agShortcut->addAction(linkAction(ui->maAssignShortcut, TYON_BUTTON_TYPE_SHORTCUT));
    agShortcut->addAction(linkAction(ui->maAssignReset, TYON_BUTTON_TYPE_UNUSED));
    m_actions[tr("2 Shortcut")] = agShortcut;

    QActionGroup *agDPICycle = new QActionGroup(this);
    agDPICycle->addAction(linkAction(ui->maAssignDPIUp, TYON_BUTTON_TYPE_CPI_UP));
    agDPICycle->addAction(linkAction(ui->maAssignDPIDown, TYON_BUTTON_TYPE_CPI_DOWN));
    agDPICycle->addAction(linkAction(ui->maAssignDPICycle, TYON_BUTTON_TYPE_CPI_CYCLE));
    m_actions[tr("3 Sensor DPI")] = agDPICycle;

    QActionGroup *agSensitivity = new QActionGroup(this);
    agSensitivity->addAction(linkAction(ui->maAssignSensitivityUp, TYON_BUTTON_TYPE_SENSITIVITY_UP));
    agSensitivity->addAction(linkAction(ui->maAssignSensitivityDown, TYON_BUTTON_TYPE_SENSITIVITY_DOWN));
    agSensitivity->addAction(linkAction(ui->maAssignSensitivityCycle, TYON_BUTTON_TYPE_SENSITIVITY_CYCLE));
    m_actions[tr("4 Sensitivity")] = agSensitivity;

    QActionGroup *agMultimedia = new QActionGroup(this);
    agMultimedia->addAction(linkAction(ui->maAssignMMOpen, TYON_BUTTON_TYPE_OPEN_PLAYER));
    agMultimedia->addAction(linkAction(ui->maAssignMMNextTrack, TYON_BUTTON_TYPE_PREV_TRACK));
    agMultimedia->addAction(linkAction(ui->maAssignMMPreviousTrack, TYON_BUTTON_TYPE_NEXT_TRACK));
    agMultimedia->addAction(linkAction(ui->maAssignMMPlay, TYON_BUTTON_TYPE_PLAY_PAUSE));
    agMultimedia->addAction(linkAction(ui->maAssignMMStop, TYON_BUTTON_TYPE_STOP));
    agMultimedia->addAction(linkAction(ui->maAssignMMMute, TYON_BUTTON_TYPE_MUTE));
    agMultimedia->addAction(linkAction(ui->maAssignMMVolumeUp, TYON_BUTTON_TYPE_VOLUME_UP));
    agMultimedia->addAction(linkAction(ui->maAssignMMVolumeDown, TYON_BUTTON_TYPE_VOLUME_DOWN));
    m_actions[tr("5 Multimedia")] = agMultimedia;

    QActionGroup *agProfile = new QActionGroup(this);
    agProfile->addAction(linkAction(ui->maAssignProfileUp, TYON_BUTTON_TYPE_PROFILE_UP));
    agProfile->addAction(linkAction(ui->maAssignProfileDown, TYON_BUTTON_TYPE_PROFILE_DOWN));
    m_actions[tr("6 Profiles")] = agProfile;

    QPushButton *pb;
    foreach (auto o, ui->pnlTopButtons->children()) {
        if ((pb = dynamic_cast<QPushButton *>(o)) != nullptr) {
            linkButton(pb, m_actions);
        }
    }
    foreach (auto o, ui->pnlTopEasyShift->children()) {
        if ((pb = dynamic_cast<QPushButton *>(o)) != nullptr) {
            linkButton(pb, m_actions);
        }
    }
    foreach (auto o, ui->pnlSideButtons->children()) {
        if ((pb = dynamic_cast<QPushButton *>(o)) != nullptr) {
            linkButton(pb, m_actions);
        }
    }
    foreach (auto o, ui->pnlSideEasyShift->children()) {
        if ((pb = dynamic_cast<QPushButton *>(o)) != nullptr) {
            linkButton(pb, m_actions);
        }
    }
}

inline void RTMainWindow::connectController()
{
    connect(m_ctlr, &RTDeviceController::lookupStarted, this, &RTMainWindow::onLookupStarted);
    connect(m_ctlr, &RTDeviceController::deviceFound, this, &RTMainWindow::onDeviceFound);
    connect(m_ctlr, &RTDeviceController::deviceRemoved, this, &RTMainWindow::onDeviceRemoved);
    connect(m_ctlr, &RTDeviceController::deviceError, this, &RTMainWindow::onDeviceError);
    connect(m_ctlr, &RTDeviceController::deviceInfoChanged, this, &RTMainWindow::onDeviceInfo);
    connect(m_ctlr, &RTDeviceController::profileIndexChanged, this, &RTMainWindow::onProfileIndex);
    connect(m_ctlr, &RTDeviceController::settingsChanged, this, &RTMainWindow::onSettingsChanged);
    connect(m_ctlr, &RTDeviceController::buttonsChanged, this, &RTMainWindow::onButtonsChanged);
    connect(m_ctlr, &RTDeviceController::saveProfilesStarted, this, &RTMainWindow::onSaveProfilesStarted);
    connect(m_ctlr, &RTDeviceController::saveProfilesFinished, this, &RTMainWindow::onSaveProfilesFinished);
}

inline void RTMainWindow::connectActions()
{
    connect(ui->pbImport, &QPushButton::clicked, this, [this](bool) { //
        QString fileName;
        if (selectFile(fileName, true)) {
            m_ctlr->loadProfilesFromFile(fileName);
        }
    });
    connect(ui->pbExport, &QPushButton::clicked, this, [this](bool) { //
        QString fileName;
        if (selectFile(fileName, false)) {
            m_ctlr->saveProfilesToFile(fileName);
        }
    });
    connect(ui->pbReset, &QPushButton::clicked, this, [this](bool) { //
        const QString title = tr("Device reset");
        const QString msg = tr("Warning!\nAny changes will be lost.\n\n" //
                               "Do you want to reset device?\n");
        if (QMessageBox::question(this, title, msg) == QMessageBox::Yes) {
            RTProgress::present(tr("Please wait..."), this);
            if (!m_ctlr->resetProfiles()) {
                QMessageBox::warning( //
                    this,
                    title,
                    tr("Unable to reset device profiles.\n"
                       "Please close the application and try again."));
                ui->pnlLeft->setEnabled(false);
                ui->tabWidget->setEnabled(false);
                ui->pbSave->setEnabled(false);
            }
        }
    });
    connect(ui->pbSave, &QPushButton::clicked, this, [this](bool) { //
        const QString title = tr("Save device profiles");
        const QString msg = tr("Do you want to updat device profiles?\n");
        if (QMessageBox::question(this, title, msg) == QMessageBox::Yes) {
            if (!m_ctlr->saveProfilesToDevice()) {
                QMessageBox::warning(this, title, tr("Unable to save device profiles."));
            }
        }
    });
}

inline void RTMainWindow::connectUiElements()
{
    connect(ui->tableView, &QTableView::clicked, this, [this](const QModelIndex &index) { //
        m_ctlr->selectProfile(index.row());
    });
    connect(ui->tableView, &QTableView::activated, this, [](const QModelIndex &) { //
        // double click ...
    });

    // synchronize X/Y sensitivity if advanced disabled
    connect(ui->hsXSensitivity, &QSlider::valueChanged, this, [this](int position) { //
        ui->edXSensitivity->setValue(position);
        if (!ui->cbxSenitivityEnableAdv->isChecked()) {
            ui->edYSensitivity->setValue(position); // trigers slider too
        }
        m_ctlr->setXSensitivity(position);
    });
    connect(ui->edXSensitivity, &QSpinBox::valueChanged, this, [this](int position) { //
        ui->hsXSensitivity->setValue(position);
        if (!ui->cbxSenitivityEnableAdv->isChecked()) {
            ui->hsYSensitivity->setValue(position); // trigers slider too
        }
        m_ctlr->setXSensitivity(position);
    });
    connect(ui->hsYSensitivity, &QSlider::valueChanged, this, [this](int position) { //
        ui->edYSensitivity->setValue(position);
        if (!ui->cbxSenitivityEnableAdv->isChecked()) {
            ui->edXSensitivity->setValue(position); // trigers slider too
        }
        m_ctlr->setYSensitivity(position);
    });
    connect(ui->edYSensitivity, &QSpinBox::valueChanged, this, [this](int position) { //
        ui->hsYSensitivity->setValue(position);
        if (!ui->cbxSenitivityEnableAdv->isChecked()) {
            ui->hsXSensitivity->setValue(position); // trigers slider too
        }
        m_ctlr->setYSensitivity(position);
    });

    connect(ui->cbxSenitivityEnableAdv, &QCheckBox::clicked, this, [this](bool checked) { //
        m_ctlr->setAdvancedSenitivity(ROCCAT_SENSITIVITY_ADVANCED_ON, checked);
    });

    connect(ui->rbPollRate125, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setPollRate(ROCCAT_POLLING_RATE_125);
    });
    connect(ui->rbPollRate250, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setPollRate(ROCCAT_POLLING_RATE_250);
    });
    connect(ui->rbPollRate500, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setPollRate(ROCCAT_POLLING_RATE_500);
    });
    connect(ui->rbPollRate1000, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setPollRate(ROCCAT_POLLING_RATE_1000);
    });

    connect(ui->cbxDpiSlot1, &QCheckBox::clicked, this, [this](bool checked) { //
        ui->edDpiSlot1->setEnabled(checked);
        ui->hsDpiSlot1->setEnabled(checked);
        m_ctlr->setDpiSlot(0x01, checked);
    });
    connect(ui->cbxDpiSlot2, &QCheckBox::clicked, this, [this](bool checked) { //
        ui->edDpiSlot2->setEnabled(checked);
        ui->hsDpiSlot2->setEnabled(checked);
        m_ctlr->setDpiSlot(0x02, checked);
    });
    connect(ui->cbxDpiSlot3, &QCheckBox::clicked, this, [this](bool checked) { //
        ui->edDpiSlot3->setEnabled(checked);
        ui->hsDpiSlot3->setEnabled(checked);
        m_ctlr->setDpiSlot(0x04, checked);
    });
    connect(ui->cbxDpiSlot4, &QCheckBox::clicked, this, [this](bool checked) { //
        ui->edDpiSlot4->setEnabled(checked);
        ui->hsDpiSlot4->setEnabled(checked);
        m_ctlr->setDpiSlot(0x08, checked);
    });
    connect(ui->cbxDpiSlot5, &QCheckBox::clicked, this, [this](bool checked) { //
        ui->edDpiSlot5->setEnabled(checked);
        ui->hsDpiSlot5->setEnabled(checked);
        m_ctlr->setDpiSlot(0x10, checked);
    });

    connect(ui->cbxDPIActiveSlot, &QComboBox::activated, this, [this](int index) { //
        QVariant v = ui->cbxDPIActiveSlot->itemData(index, Qt::UserRole);
        if (!v.isNull() && v.isValid()) {
            m_ctlr->setActiveDpiSlot(v.toUInt());
        }
    });

    quint8 sbIndex = 0;
    foreach (QSpinBox *c, ui->pnlSensorDpi->findChildren<QSpinBox *>(Qt::FindChildrenRecursively)) {
        connect(c, &QSpinBox::valueChanged, this, [this, c, sbIndex](int value) { //
            QSlider *s;
            QVariant v = c->property("link");
            if (v.isValid() && (s = v.value<QSlider *>())) {
                s->setValue(value);
            }
            m_ctlr->setDpiLevel(sbIndex, value);
        });
        sbIndex++;
    }

    quint8 hsIndex = 0;
    foreach (QSlider *c, ui->pnlSensorDpi->findChildren<QSlider *>(Qt::FindChildrenRecursively)) {
        connect(c, &QSlider::valueChanged, this, [this, c, hsIndex](int value) { //
            QSpinBox *b;
            QVariant v = c->property("link");
            if (v.isValid() && (b = v.value<QSpinBox *>())) {
                b->setValue(value);
            }
            m_ctlr->setDpiLevel(hsIndex, value);
        });
        hsIndex++;
    }

    connect(ui->edDpiSlot2, &QSpinBox::valueChanged, this, [this](int value) { //
        ui->hsDpiSlot2->setValue(value);
        m_ctlr->setDpiLevel(1, value);
    });
    connect(ui->edDpiSlot3, &QSpinBox::valueChanged, this, [this](int value) { //
        ui->hsDpiSlot3->setValue(value);
        m_ctlr->setDpiLevel(2, value);
    });
    connect(ui->edDpiSlot4, &QSpinBox::valueChanged, this, [this](int value) { //
        ui->hsDpiSlot4->setValue(value);
        m_ctlr->setDpiLevel(3, value);
    });
    connect(ui->edDpiSlot5, &QSpinBox::valueChanged, this, [this](int value) { //
        ui->hsDpiSlot5->setValue(value);
        m_ctlr->setDpiLevel(4, value);
    });

    connect(ui->rbLightsOff, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setLightsEffect(TYON_PROFILE_SETTINGS_LIGHT_EFFECT_ALL_OFF);
    });
    connect(ui->rbLightFullOn, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setLightsEffect(TYON_PROFILE_SETTINGS_LIGHT_EFFECT_FULLY_LIGHTED);
    });
    connect(ui->rbLightBlink, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setLightsEffect(TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BLINKING);
    });
    connect(ui->rbLightBreath, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setLightsEffect(TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BREATHING);
    });
    connect(ui->rbLightBeat, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setLightsEffect(TYON_PROFILE_SETTINGS_LIGHT_EFFECT_HEARTBEAT);
    });

    connect(ui->rbLightPaletteColor, &QRadioButton::clicked, this, [this](bool checked) { //
        if (checked) {
            m_ctlr->setLightsEnabled(TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR, false);
        }
    });
    connect(ui->rbLightCustomColor, &QRadioButton::clicked, this, [this](bool checked) { //
        if (checked) {
            m_ctlr->setLightsEnabled(TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR, true);
        }
    });

    connect(ui->rbColorNoFlow, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setColorFlow(TYON_PROFILE_SETTINGS_COLOR_FLOW_OFF);
        ui->gbxLightColor->setEnabled(true);
    });
    connect(ui->rbColorAllLights, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setColorFlow(TYON_PROFILE_SETTINGS_COLOR_FLOW_SIMULTANEOUSLY);
        ui->gbxLightColor->setEnabled(false);
    });
    connect(ui->rbColorDirUp, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setColorFlow(TYON_PROFILE_SETTINGS_COLOR_FLOW_UP);
        ui->gbxLightColor->setEnabled(false);
    });
    connect(ui->rbColorDirDown, &QRadioButton::clicked, this, [this](bool) { //
        m_ctlr->setColorFlow(TYON_PROFILE_SETTINGS_COLOR_FLOW_DOWN);
        ui->gbxLightColor->setEnabled(false);
    });

    // -- gbxLightColor
    connect(ui->cbxLightWheel, &QCheckBox::clicked, this, [this](bool checked) { //
        m_ctlr->setLightsEnabled(TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_WHEEL, checked);
    });
    connect(ui->pbLightColorWheel, &QPushButton::clicked, this, [this]() { //
        TyonLight color;
        if (selectColor(TYON_LIGHT_WHEEL, color)) {
            m_ctlr->setLightColor(TYON_LIGHT_WHEEL, color);
        }
    });
    connect(ui->cbxLightBottom, &QCheckBox::clicked, this, [this](bool checked) { //
        m_ctlr->setLightsEnabled(TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_BOTTOM, checked);
    });
    connect(ui->pbLightColorBottom, &QPushButton::clicked, this, [this]() { //
        TyonLight color;
        if (selectColor(TYON_LIGHT_BOTTOM, color)) {
            m_ctlr->setLightColor(TYON_LIGHT_BOTTOM, color);
        }
    });
}

inline bool RTMainWindow::selectColor(TyonLightType target, TyonLight &color)
{
    if (ui->rbLightCustomColor->isChecked()) {
        QColorDialog d(this);
        d.setOption(QColorDialog::ColorDialogOption::DontUseNativeDialog);
        d.setTabletTracking(this->hasTabletTracking());
        if (d.exec() == QColorDialog::Accepted) {
            color = m_ctlr->toDeviceColor(target, d.selectedColor());
            return true;
        }
    } else {
        // show ROCCAT Tyon colors
        RTColorDialog d(m_ctlr->deviceColors(), this);
        if (d.exec() == RTColorDialog::Accepted) {
            color = d.selectedColor();
            return true;
        }
    }
    return false;
}

inline bool RTMainWindow::selectFile(QString &file, bool isOpen)
{
    QString path = QStandardPaths::writableLocation( //
        QStandardPaths::DocumentsLocation);
    file = QDir::toNativeSeparators(path + "/" + m_rtpfFileName);

    QFileDialog d(this);
    connect(&d, &QFileDialog::fileSelected, this, [](const QString &file) { //
        qDebug("[APPWIN] File selected: %s", qPrintable(file));
    });
    connect(&d, &QFileDialog::directoryEntered, this, [](const QString &directory) { //
        qDebug("[APPWIN] Directory entered: %s", qPrintable(directory));
    });
    d.setWindowTitle(tr(isOpen ? "Open profile" : "Save profile as"));
    d.setWindowFilePath(file);
    d.setAcceptMode(isOpen //
                        ? QFileDialog::AcceptMode::AcceptOpen
                        : QFileDialog::AcceptMode::AcceptSave);
    d.setFileMode(QFileDialog::FileMode::AnyFile);
    d.setFilter(QDir::NoFilter);
    d.setDirectory(file);
    d.setLabelText(QFileDialog::DialogLabel::FileName, tr("Profile name:"));
    d.setLabelText(QFileDialog::DialogLabel::FileType, tr("Extension:"));
    d.setOption(QFileDialog::Option::ReadOnly, isOpen ? true : false);
    d.setOption(QFileDialog::Option::DontConfirmOverwrite, false);
    d.setOption(QFileDialog::Option::DontUseNativeDialog, false);
    d.setNameFilters(QStringList() << "*.rtpf" << "*.*");
    d.setHistory(QStringList() << file);
    d.setDefaultSuffix(".rtpf");
    if (d.exec() == QFileDialog::Accepted) {
        if (!d.selectedFiles().isEmpty()) {
            file = d.selectedFiles().at(0);
            return true;
        }
    }
    return false;
}

inline QAction *RTMainWindow::linkAction(QAction *action, TyonButtonType function)
{
    connect(action, &QAction::triggered, this, [this, action, function](bool) { //
        if (!action->isEnabled()) {
            return;
        }

        /* the push button object is set by its event handler */
        QVariant v = action->property("button");
        if (v.isNull() || !v.isValid()) {
            return;
        }
        QPushButton *pb = v.value<QPushButton *>();
        if (!pb) {
            return;
        }

        //qDebug() << "--------------------------";
        //qDebug() << "[APPWIN] triggered():" << function << action;
        //qDebug() << "[APPWIN] triggered():" << pb << pb->text();

        RTDeviceController::TSetButtonCallback handler;
        if (m_buttons.contains(pb)) {
            const RTDeviceController::TButtonLink bl = m_buttons[pb];
            if ((handler = bl.handler) != nullptr) {
                if (function != TYON_BUTTON_TYPE_SHORTCUT) {
                    handler(bl.index, function, {});
                } else {
                    RTShortcutDialog d(this);
                    if (d.exec() != QDialog::Accepted) {
                        return;
                    }
                    handler(bl.index, function, d.data().keyCombo);
                }
            } else {
                qCritical() << "[APPWIN] ROCCAT handler NULL pointer.";
            }
        } else {
            qCritical() << "[APPWIN] Given push button not found.";
        }
    });

    return action;
}

inline void RTMainWindow::linkButton(QPushButton *pb, const QMap<QString, QActionGroup *> &actions)
{
    connect(pb, &QPushButton::clicked, this, [pb, actions](bool) { //
        const QList<QString> groupNames = actions.keys();
        QMenu *popupMenu = new QMenu(pb);
        connect(popupMenu, &QMenu::aboutToHide, popupMenu, &QMenu::deleteLater);
        foreach (const QString key, groupNames) {
            QMenu *subMenu = new QMenu(key, pb);
            foreach (QAction *action, actions[key]->actions()) {
                /* assign calling push button to menu action which is
                 * referenced in action event handler */
                action->setProperty("button", QVariant::fromValue(pb));
                subMenu->addAction(action);
            }
            popupMenu->addMenu(subMenu);
        }
        /* Retrieve a valid width of the menu. (It's not
         * the same as using "pMenu->width()"!) */
        int menuWidth = popupMenu->sizeHint().width();
        QPoint pos(pb->mapToGlobal(QPoint(pb->width() - menuWidth, pb->height())));
        /* show popup menu at specifc position */
        popupMenu->popup(pos);
    });
}

static int progress_count = 0;
void RTMainWindow::onLookupStarted()
{
    ui->pnlLeft->setEnabled(false);
    ui->tabWidget->setEnabled(false);
    ui->pbSave->setEnabled(false);
    ui->pbReset->setEnabled(false);

    progress_count = 0;
    RTProgress::present(tr("Searching ROCCAT device..."), this);

    // timeout timer
    QTimer::singleShot(10000, this, [this]() {
        if (!m_ctlr->hasDevice()) {
            QMessageBox::warning( //
                this,
                qApp->applicationDisplayName(),
                tr("Unable to find ROCCAT Tyon device."));
            RTProgress::dismiss();
            QTimer::singleShot(500, this, []() { qApp->quit(); });
            return;
        }
    });

    QThread::yieldCurrentThread();
}

void RTMainWindow::onDeviceFound()
{
    ui->pnlLeft->setEnabled(true);
    ui->tabWidget->setEnabled(true);
    ui->pbSave->setEnabled(true);
    ui->pbReset->setEnabled(true);
}

void RTMainWindow::onDeviceRemoved()
{
    ui->pnlLeft->setEnabled(false);
    ui->tabWidget->setEnabled(false);
    ui->pbSave->setEnabled(false);
    ui->pbReset->setEnabled(false);
    ui->pbClose->setFocus();

    setWindowTitle(qApp->applicationDisplayName());
    onProfileIndex(0);

    RTProgress::present(tr("Searching ROCCAT device..."), this);
}

void RTMainWindow::onDeviceError(uint error, const QString &message)
{
    qCritical("[APPWIN] ERROR %d: %s", error, qPrintable(message));

    QMessageBox::warning(this,
                         qApp->applicationDisplayName(), //
                         tr("ERROR 0x%1: %2")            //
                             .arg(error, 8, 16, QChar('0'))
                             .arg(message));

    RTProgress::dismiss();
    ui->pnlLeft->setEnabled(false);
    ui->tabWidget->setEnabled(false);
    ui->pbSave->setEnabled(false);
    ui->pbReset->setEnabled(true);
    ui->pbReset->setFocus();
    onProfileIndex(0);
}

void RTMainWindow::onDeviceInfo(const TyonInfo &info)
{
    setWindowTitle(           //
        tr("%1 FW:%2 DFU:%3") //
            .arg(qApp->applicationDisplayName())
            .arg(QString::number((qreal) (info.firmware_version * 0.01), 'f', 2))
            .arg(QString::number((qreal) (info.dfu_version * 0.01), 'f', 2)));
}

void RTMainWindow::onProfileIndex(const quint8 pix)
{
    m_activeProfile = pix;

    ui->tableView->setCurrentIndex(m_ctlr->profileIndex(pix));
    statusBar()->showMessage(tr("Active profile: %1").arg(pix));
}

/* Assign ROCCAT profile settings to UI elements */
void RTMainWindow::onSettingsChanged(const TyonProfileSettings &settings)
{
    const TyonProfileSettings *s = &settings;

    /* skip */
    if (s->profile_index != m_activeProfile) {
        return;
    }

    ui->cbxSenitivityEnableAdv->setChecked(s->advanced_sensitivity & ROCCAT_SENSITIVITY_ADVANCED_ON);

    if (ui->cbxSenitivityEnableAdv->isChecked()) {
        ui->edXSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
        ui->hsXSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
        ui->edYSensitivity->setValue(m_ctlr->toSensitivityYValue(s));
        ui->hsYSensitivity->setValue(m_ctlr->toSensitivityYValue(s));
    } else {
        ui->edXSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
        ui->hsXSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
        ui->edYSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
        ui->hsYSensitivity->setValue(m_ctlr->toSensitivityXValue(s));
    }

    ui->rbPollRate125->setChecked(s->talkfx_polling_rate == ROCCAT_POLLING_RATE_125);
    ui->rbPollRate250->setChecked(s->talkfx_polling_rate == ROCCAT_POLLING_RATE_250);
    ui->rbPollRate500->setChecked(s->talkfx_polling_rate == ROCCAT_POLLING_RATE_500);
    ui->rbPollRate1000->setChecked(s->talkfx_polling_rate == ROCCAT_POLLING_RATE_1000);

    ui->cbxDpiSlot1->setChecked(s->cpi_levels_enabled & 0x01);
    ui->cbxDpiSlot2->setChecked(s->cpi_levels_enabled & 0x02);
    ui->cbxDpiSlot3->setChecked(s->cpi_levels_enabled & 0x04);
    ui->cbxDpiSlot4->setChecked(s->cpi_levels_enabled & 0x08);
    ui->cbxDpiSlot5->setChecked(s->cpi_levels_enabled & 0x10);

    ui->cbxDPIActiveSlot->clear();
    for (qint8 i = 0; i < TYON_PROFILE_SETTINGS_CPI_LEVELS_NUM; i++) {
        if (s->cpi_levels_enabled & (1 << i)) {
            ui->cbxDPIActiveSlot->addItem(tr("Slot %1").arg(i + 1), QVariant(i));
        }
        switch (i) {
            case 0: {
                ui->edDpiSlot1->setValue(m_ctlr->toDpiLevelValue(s, i));
                ui->hsDpiSlot1->setValue(m_ctlr->toDpiLevelValue(s, i));
                break;
            }
            case 1: {
                ui->edDpiSlot2->setValue(m_ctlr->toDpiLevelValue(s, i));
                ui->hsDpiSlot2->setValue(m_ctlr->toDpiLevelValue(s, i));
                break;
            }
            case 2: {
                ui->edDpiSlot3->setValue(m_ctlr->toDpiLevelValue(s, i));
                ui->hsDpiSlot3->setValue(m_ctlr->toDpiLevelValue(s, i));
                break;
            }
            case 3: {
                ui->edDpiSlot4->setValue(m_ctlr->toDpiLevelValue(s, i));
                ui->hsDpiSlot4->setValue(m_ctlr->toDpiLevelValue(s, i));
                break;
            }
            case 4: {
                ui->edDpiSlot5->setValue(m_ctlr->toDpiLevelValue(s, i));
                ui->hsDpiSlot5->setValue(m_ctlr->toDpiLevelValue(s, i));
                break;
            }
        }
    }
    if (ui->cbxDPIActiveSlot->count() == 0) {
        ui->cbxDPIActiveSlot->setEnabled(false);
    } else if (s->cpi_active < ui->cbxDPIActiveSlot->count()) {
        ui->cbxDPIActiveSlot->setCurrentIndex(s->cpi_active);
    } else {
        quint8 index = ui->cbxDPIActiveSlot->count() - 1;
        ui->cbxDPIActiveSlot->setCurrentIndex(index);
        QTimer::singleShot(50, this, [this, index]() { //
            QVariant v = ui->cbxDPIActiveSlot->itemData(index, Qt::UserRole);
            if (!v.isNull() && v.isValid()) {
                m_ctlr->setActiveDpiSlot(v.toUInt());
            }
        });
    }

    ui->rbLightCustomColor->setChecked(s->lights_enabled & TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_CUSTOM_COLOR);
    ui->rbLightPaletteColor->setChecked(!ui->rbLightCustomColor->isChecked());
    ui->cbxLightWheel->setChecked(s->lights_enabled & TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_WHEEL);
    ui->cbxLightBottom->setChecked(s->lights_enabled & TYON_PROFILE_SETTINGS_LIGHTS_ENABLED_BIT_BOTTOM);

    ui->rbLightsOff->setChecked(s->light_effect == TYON_PROFILE_SETTINGS_LIGHT_EFFECT_ALL_OFF);
    ui->rbLightFullOn->setChecked(s->light_effect == TYON_PROFILE_SETTINGS_LIGHT_EFFECT_FULLY_LIGHTED);
    ui->rbLightBlink->setChecked(s->light_effect == TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BLINKING);
    ui->rbLightBreath->setChecked(s->light_effect == TYON_PROFILE_SETTINGS_LIGHT_EFFECT_BREATHING);
    ui->rbLightBeat->setChecked(s->light_effect == TYON_PROFILE_SETTINGS_LIGHT_EFFECT_HEARTBEAT);

    ui->rbColorNoFlow->setChecked(s->color_flow == TYON_PROFILE_SETTINGS_COLOR_FLOW_OFF);
    ui->rbColorAllLights->setChecked(s->color_flow == TYON_PROFILE_SETTINGS_COLOR_FLOW_SIMULTANEOUSLY);
    ui->rbColorDirUp->setChecked(s->color_flow == TYON_PROFILE_SETTINGS_COLOR_FLOW_UP);
    ui->rbColorDirDown->setChecked(s->color_flow == TYON_PROFILE_SETTINGS_COLOR_FLOW_DOWN);

    for (qint8 i = 0; i < TYON_LIGHTS_NUM; i++) {
        QColor color = m_ctlr->toScreenColor(s->lights[i], ui->rbLightCustomColor->isChecked());
        switch (i) {
            case 0: {
                ui->pbLightColorWheel->setStyleSheet(        //
                    tr("background-color: rgb(%1, %2, %3);") //
                        .arg(color.red())                    //
                        .arg(color.green())
                        .arg(color.blue()));
                break;
            }
            case 1: {
                ui->pbLightColorBottom->setStyleSheet(       //
                    tr("background-color: rgb(%1, %2, %3);") //
                        .arg(color.red())                    //
                        .arg(color.green())
                        .arg(color.blue()));
                break;
            }
        }
    }
}

/* Assign ROCCAT button actions to UI push buttons */
void RTMainWindow::onButtonsChanged(const TyonProfileButtons &buttons)
{
    if (ui->tableView->model()->rowCount({}) >= TYON_PROFILE_NUM) {
        RTProgress::dismiss();
    }

    /* skip */
    if (buttons.profile_index != m_activeProfile) {
        return;
    }

    /* physical buttons and with EasyShift combined. 32 buttons (16x2) */
    const quint8 idxStart = TYON_BUTTON_INDEX_LEFT;
    const quint8 idxCount = TYON_PROFILE_BUTTON_NUM;
    const TyonProfileButtons *b = &buttons;

    auto toPushButton = [this](quint8 index) -> QPushButton * {
        const QList<QPushButton *> pbkeys = m_buttons.keys();
        foreach (QPushButton *pb, pbkeys) {
            RTDeviceController::TButtonLink link = m_buttons[pb];
            if (link.index == index) {
                return pb;
            }
        }
        return nullptr;
    };

    QPushButton *pb = nullptr;
    for (quint8 index = idxStart; index < idxCount; index++) {
        if ((pb = toPushButton(index)) != nullptr) {
            m_ctlr->setupButton(b->buttons[index], pb);
        }
    }
}

void RTMainWindow::onSaveProfilesStarted()
{
    RTProgress::present(tr("Please wait..."), this);
    ui->pnlContent->setEnabled(false);
    ui->pnlLeft->setEnabled(false);
}

void RTMainWindow::onSaveProfilesFinished()
{
    ui->pnlContent->setEnabled(true);
    ui->pnlLeft->setEnabled(true);
    RTProgress::dismiss();
}
