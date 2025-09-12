// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rttcuimagewidget.h"
#include <QPainter>
#include <QStyleOption>

RTTcuImageWidget::RTTcuImageWidget(QWidget *parent)
    : QWidget(parent)
    , m_margin(4)
    , m_pixelSize(4)
    , m_pixelColor(Qt::black)
    , m_imageSize(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

// Daten setzen
void RTTcuImageWidget::setImageData(const QVector<quint8> &data, int size)
{
    m_imageData = data;
    m_imageSize = size;
    update();
}

// Setter
void RTTcuImageWidget::setMargin(int margin)
{
    if (m_margin != margin) {
        m_margin = margin;
        emit marginChanged(m_margin);
        update();
    }
}

void RTTcuImageWidget::setPixelSize(int size)
{
    if (m_pixelSize != size) {
        m_pixelSize = size;
        emit pixelSizeChanged(m_pixelSize);
        update();
    }
}

void RTTcuImageWidget::setPixelColor(const QColor &color)
{
    if (m_pixelColor != color) {
        m_pixelColor = color;
        emit pixelColorChanged(m_pixelColor);
        update();
    }
}

void RTTcuImageWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QStyleOption opt;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    opt.initFrom(this);
#else
    opt.init(this);
#endif

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (m_imageData.isEmpty() || m_imageSize == 0)
        return;

    // Zeichenbereich
    QRect area = rect().adjusted(m_margin, m_margin, -m_margin, -m_margin);

    for (int l = rect().y(); l < (rect().y() + rect().height() - m_margin); l++) {
        area.moveTo(rect().x() + m_margin, area.y() + m_pixelSize);

        for (int x = 0; x < m_imageSize; ++x) {
            for (int y = 0; y < m_imageSize; ++y) {
                int idx = y * m_imageSize + (m_imageSize - 1 - x);
                if (idx < 0 || idx >= m_imageData.size())
                    continue;

                quint16 color_value = m_imageData[idx];
                color_value = (color_value * 4 + 24) * 256;
                if (color_value > 65535)
                    color_value = 65535;

                QColor c = m_pixelColor;
                c.setRed(color_value >> 8);
                c.setGreen(color_value >> 8);
                c.setBlue(color_value >> 8);

                QRect pixelRect(area.left() + x * m_pixelSize, //
                                area.top() + y * m_pixelSize,
                                m_pixelSize,
                                m_pixelSize);

                painter.fillRect(pixelRect, c);
            }
        }
    }
}
