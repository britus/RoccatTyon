#pragma once
#include "rtdevicecontroller.h"
#include "rtprofilemodel.h"
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
    void onSettingsChanged(const TyonProfileSettings &settings);
    void onButtonsChanged(const TyonProfileButtons &buttons);
    void onDeviceInfo(const TyonInfo &info);
    void onProfileIndex(const quint8 pix);

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

private:
    inline void initializeUiElements();
    inline void initializeSettings();
    inline void connectController();
    inline void connectUiElements();
    inline void connectActions();
    inline void loadSettings(QSettings *settings);
    inline void saveSettings(QSettings *settings);
    inline void linkButton(QPushButton *pb, const QMap<QString, QActionGroup *> &actions);
    inline QAction *linkAction(QAction *action, TyonButtonType function);
    inline bool selectColor(QColor &color);
    inline bool selectFile(QString &file, bool isOpen = true);
};
