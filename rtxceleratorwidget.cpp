// ********************************************************************
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// Copyright by libgaminggear Project (some copied parts)
// Copyright by roccat-tools Project (some copied parts)
// SPDX-License-Identifier: GPL-3.0
// ********************************************************************
#include "rtxceleratorwidget.h"
#include <QEasingCurve>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOption>

RTXCeleratorWidget::RTXCeleratorWidget(QWidget *parent)
    : QWidget(parent)
    , m_padding(10)
    , m_lineWidth(4)
    , m_squareSize(16)
    , m_value(128)
    , m_minimum(255) /* bottom */
    , m_maximum(0)   /* top */
    , m_animatedValue(128)
{
    setMinimumSize(60, 100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_anim = new QPropertyAnimation(this, "animatedValue", this);
    m_anim->setDuration(300); // ms
    m_anim->setEasingCurve(QEasingCurve::InOutQuad);

    m_updateTimer.start();
}

int RTXCeleratorWidget::padding() const
{
    return m_padding;
}
void RTXCeleratorWidget::setPadding(int p)
{
    m_padding = p;
    update();
}

int RTXCeleratorWidget::lineWidth() const
{
    return m_lineWidth;
}
void RTXCeleratorWidget::setLineWidth(int w)
{
    m_lineWidth = w;
    update();
}

int RTXCeleratorWidget::squareSize() const
{
    return m_squareSize;
}
void RTXCeleratorWidget::setSquareSize(int s)
{
    m_squareSize = s;
    update();
}

int RTXCeleratorWidget::value() const
{
    return m_value;
}
void RTXCeleratorWidget::setValue(int v)
{
    v = qBound(m_maximum, v, m_minimum); // invertierte Logik
    if (m_value != v) {
        m_value = v;
        emit valueChanged(m_value);

        // Falls Updates extrem schnell eintreffen, sofort setzen (keine Animation)
        if (m_updateTimer.elapsed() < 30) { // < 30ms seit letzter Änderung
            m_anim->stop();
            setAnimatedValue(m_value);
        } else {
            // Saubere Animation starten
            m_anim->stop();
            m_anim->setStartValue(m_animatedValue);
            m_anim->setEndValue(m_value);
            m_anim->start();
        }
        m_updateTimer.restart();
    }
}

int RTXCeleratorWidget::minimum() const
{
    return m_minimum;
}
void RTXCeleratorWidget::setMinimum(int min)
{
    if (m_minimum != min) {
        m_minimum = min;
        if (m_value > m_minimum)
            setValue(m_minimum);
        emit minimumChanged(m_minimum);
        update();
    }
}

int RTXCeleratorWidget::maximum() const
{
    return m_maximum;
}
void RTXCeleratorWidget::setMaximum(int max)
{
    if (m_maximum != max) {
        m_maximum = max;
        if (m_value < m_maximum)
            setValue(m_maximum);
        emit maximumChanged(m_maximum);
        update();
    }
}

int RTXCeleratorWidget::animatedValue() const
{
    return m_animatedValue;
}
void RTXCeleratorWidget::setAnimatedValue(int v)
{
    if (m_animatedValue != v) {
        m_animatedValue = v;
        update();
    }
}

void RTXCeleratorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStyleOption opt;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    opt.initFrom(this);
#else
    opt.init(this);
#endif

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Hintergrund (Stylesheet)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    QRect rect = this->rect().adjusted(m_padding, m_padding, -m_padding, -m_padding);

    // Linie in der Mitte
    int centerX = rect.center().x();
    QRect lineRect(centerX - m_lineWidth / 2, rect.top(), m_lineWidth, rect.height());

    QColor lineColor = palette().color(QPalette::WindowText);
    painter.fillRect(lineRect, lineColor);

    // Invertierte Normalisierung (max oben, min unten)
    double ratio = 0.0;
    if (m_minimum != m_maximum) {
        ratio = static_cast<double>(m_animatedValue - m_maximum) / (m_minimum - m_maximum);
    }

    int squareY = rect.top() + static_cast<int>(ratio * (rect.height() - m_squareSize));
    QRect squareRect(centerX - m_squareSize / 2, squareY, m_squareSize, m_squareSize);

    QColor squareColor = palette().color(QPalette::Highlight);
    painter.fillRect(squareRect, squareColor);
}
