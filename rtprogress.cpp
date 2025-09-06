// RTProgress.cpp
#include "RTProgress.h"
#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QVBoxLayout>

QPointer<RTProgress> RTProgress::instance = nullptr;
QMutex RTProgress::mutex;

RTProgress::RTProgress(QWidget *parent)
    : QDialog(parent)
    , progressBar(new QProgressBar(this))
{
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *label = new QLabel("Please wait...", this);
    label->setAlignment(Qt::AlignCenter);

    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedWidth(200);

    layout->addWidget(label);
    layout->addWidget(progressBar);

    setLayout(layout);
    setFixedSize(300, 120);
#if 0
    setStyleSheet("background-color: white; "
                  "border: 1px solid gray; "
                  "border-radius: 8px; "
                  "font-size: 14px;");
#endif
}

void RTProgress::present(QWidget *parent)
{
    //QMutexLocker locker(&mutex);

    if (!instance) {
        instance = new RTProgress(parent ? parent : QApplication::activeWindow());
    }

    // Position in Bildschirmmitte
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect scr = screen->geometry();
    instance->move(scr.center() - instance->rect().center());

    instance->setVisible(true);
    //instance->raise();
    QApplication::processEvents();
}

void RTProgress::dismiss()
{
    //QMutexLocker locker(&mutex);

    if (instance) {
        instance->hide();
        instance->deleteLater();
        instance = nullptr;
    }
}

void RTProgress::setProgress(int value)
{
    //QMutexLocker locker(&mutex);

    if (instance && instance->progressBar) {
        instance->progressBar->setValue(value);
        QApplication::processEvents();
    }
}
