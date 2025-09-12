#include "rtprogress.h"
#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QStyle>
#include <QVBoxLayout>

QPointer<RTProgress> RTProgress::instance = nullptr;
QMutex RTProgress::mutex;

RTProgress::RTProgress(const QString &message, QWidget *parent)
    : QDialog(parent)
    , progressBar(new QProgressBar(this))
{
    //| Qt::WindowStaysOnTopHint
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *label = new QLabel((message.isEmpty() ? tr("Please wait...") : message), this);
    label->setAlignment(Qt::AlignCenter);

    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedWidth(200);

    layout->addWidget(label);
    layout->addWidget(progressBar);

    setLayout(layout);
    setFixedSize(300, 120);
}

void RTProgress::present(const QString &message, QWidget *parent)
{
    //QMutexLocker locker(&mutex);

    if (parent == nullptr) {
        parent = QApplication::activeWindow();
    }

    if (!instance) {
        instance = new RTProgress(message, parent);
    }

    // Position in Bildschirmmitte
    //QScreen *screen = QGuiApplication::primaryScreen();
    //QRect scr = screen->geometry();
    //instance->move(scr.center() - instance->rect().center());
    QRect r = QStyle::alignedRect( //
        Qt::LeftToRight,
        Qt::AlignCenter,
        instance->size(),
        parent->geometry());
    instance->move(r.topLeft());
    instance->resize(r.size());
    instance->setWindowTitle(message);
    instance->setVisible(true);
    instance->raise();

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
