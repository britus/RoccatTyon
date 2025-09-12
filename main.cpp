// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtmainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QLocale>
#include <QMessageBox>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QTranslator>

extern const char *GetBundleVersion();
extern const char *GetBuildNumber();

class RTApplication : public QApplication
{
public:
    RTApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {}

    int exec()
    {
        QTranslator translator;
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "RoccatTyon_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                installTranslator(&translator);
                break;
            }
        }

        QString projectFile;
        const QStringList args = arguments();
        if (args.size() > 1) {
            projectFile = args.at(1);
        }

        /* Override commandline style with our fixed GUI style type */
        /* macintosh, Windows, Fusion */
        QString styleName;
        // qDebug() << QStyleFactory::keys();
#if defined(Q_OS_MACOS)
        styleName = "Fusion"; //"macOS";
#elif defined(Q_OS_LINUX)
        styleName = "Fusion";
#else
        styleName = "Windows";
#endif
        /* configure custom GUI style hinter */
        QStyle *style;
        if ((style = QStyleFactory::create(styleName))) {
            ApplicationStyle *myStyle = new ApplicationStyle();
            myStyle->setBaseStyle(style);
            setStyle(myStyle);
        }

        QFile r(":/styles/assets/appstyle.css");
        if (r.open(QFile::ReadOnly)) {
            setStyleSheet(r.readAll());
            r.close();
        }

        m_window = new RTMainWindow(/*projectFile*/);
        m_window->show();

        int rc = QApplication::exec();

        delete m_window;
        return rc;
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            const QUrl url = openEvent->url();
            if (url.isLocalFile()) {
                QFileInfo fi(url.toLocalFile());
                qDebug() << "QEvent::FileOpen ->" << fi.absoluteFilePath();
                /*m_window->setProjectFileName(fi.absoluteFilePath());*/
            } else if (url.isValid()) {
                // process according to the URL's schema
                qDebug() << "QEvent::FileOpen ->" << url;
            } else {
                qDebug() << "QEvent::FileOpen ->" << url;
            }
        }
        return QApplication::event(event);
    }

private:
    class ApplicationStyle : public QProxyStyle
    {
    public:
        int styleHint(StyleHint hint,
                      const QStyleOption *option = nullptr, //
                      const QWidget *widget = nullptr,      //
                      QStyleHintReturn *returnData = nullptr) const override
        {
            switch (hint) {
                case QStyle::SH_ComboBox_Popup: {
                    return 0;
                }
                case QStyle::SH_MessageBox_CenterButtons: {
                    return 0;
                }
                case QStyle::SH_FocusFrame_AboveWidget: {
                    return 1;
                }
                default: {
                    break;
                }
            }
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    };

private:
    RTMainWindow *m_window;
};

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    ::setenv("QT_QPA_PLATFORM", "xcb", 0);
#endif
#ifdef Q_OS_MACOS
    ::setenv("QT_QPA_PLATFORM", "cocoa", 0);
#endif

    /* Set specific QT debug message pattern
     * %{type} %{threadid} %{function}
     * [T:%{threadid}|F:%{function}]
     */
    setenv("QT_MESSAGE_PATTERN", "[T:%{threadid}] %{message}", 0);

    QApplication::setAttribute(Qt::AA_NativeWindows, true);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
    QApplication::setAttribute(Qt::AA_Use96Dpi);

    QApplication::setOrganizationName(QStringLiteral("EoF Software Labs"));
    QApplication::setApplicationDisplayName(QStringLiteral("ROCCAT Tyon Control"));
    QApplication::setApplicationName(QStringLiteral("ROCCAT Tyon Control"));
    QApplication::setApplicationVersion(QStringLiteral("%1.%2").arg(GetBundleVersion(), GetBuildNumber()));
    QApplication::setDesktopSettingsAware(true);
    QApplication::setQuitOnLastWindowClosed(true);

    RTApplication a(argc, argv);
    return a.exec();
}
