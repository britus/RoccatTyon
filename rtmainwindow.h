// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#pragma once
#include "rtdevicecontroller.h"
#include <QAction>
#include <QActionGroup>
#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class RTMainWindow;
}
QT_END_NAMESPACE

class RTMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    RTMainWindow(QWidget *parent = nullptr);
    ~RTMainWindow();

private slots:
    void onLookupStarted();
    void onDeviceFound();
    void onDeviceRemoved();
    void onDeviceError(uint error, const QString &message);
    void onDeviceInfo(const TyonInfo &info);
    void onProfileIndex(const quint8 pix);
    void onSettingsChanged(const TyonProfileSettings &settings);
    void onButtonsChanged(const TyonProfileButtons &buttons);
    void onControlUnitChanged(const TyonControlUnit &controlUnit);
    void onTalkFxChanged(const TyonTalk &talkFx);
    void onDeviceWorkerStarted();
    void onDeviceWorkerFinished();

private:
    Ui::RTMainWindow *ui;
    /* ROCCAT Tyon controller */
    RTDeviceController *m_ctlr;
    /* Function groups which contains button menu action.
     * Each menu action has a property named 'button':
     * The property contains the calling push button */
    QMap<QString, QActionGroup *> m_actions;
    /* Link UI push button to button type and setup handler */
    QMap<QPushButton *, RTDeviceController::TButtonLink> m_buttons;
    /* Active profile of the device */
    quint8 m_activeProfile;
    /* UI settings */
    QSettings *m_settings;
    /* last export file name */
    QString m_rtpfFileName;

private:
    inline void initializeUiElements();
    inline void initializeSettings();
    inline void connectController();
    inline void connectUiElements();
    inline void connectActions();
    inline void loadSettings(QSettings *settings);
    inline void saveSettings(QSettings *settings);
    inline void enableUserInterface();
    inline void disableUserInterface();
    inline void linkButton(QPushButton *pb, const QMap<QString, QActionGroup *> &actions);
    inline QAction *linkAction(QAction *action, TyonButtonType function);
    inline bool doSelectColor(TyonLightType target, TyonLight &color);
    inline bool doSelectFile(QString &file, bool isOpen = true);
    inline void doCalibrateXCelerator();
    inline void doCalibrateTcu();
};
