#pragma once

#include <QElapsedTimer>
#include <QWidget>

class QPropertyAnimation;

class RTXCeleratorWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int padding READ padding WRITE setPadding)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(int squareSize READ squareSize WRITE setSquareSize)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(int animatedValue READ animatedValue WRITE setAnimatedValue)

public:
    explicit RTXCeleratorWidget(QWidget *parent = nullptr);

    int padding() const;
    void setPadding(int p);

    int lineWidth() const;
    void setLineWidth(int w);

    int squareSize() const;
    void setSquareSize(int s);

    int value() const;
    void setValue(int v);

    int minimum() const;
    void setMinimum(int min);

    int maximum() const;
    void setMaximum(int max);

    int animatedValue() const;
    void setAnimatedValue(int v);

signals:
    void valueChanged(int newValue);
    void minimumChanged(int newMinimum);
    void maximumChanged(int newMaximum);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_padding;
    int m_lineWidth;
    int m_squareSize;

    int m_value;
    int m_minimum;
    int m_maximum;

    int m_animatedValue;
    QPropertyAnimation *m_anim;

    QElapsedTimer m_updateTimer; // prüft Zeit zwischen Value-Änderungen
};
