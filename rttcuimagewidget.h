#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

class RTTcuImageWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int margin READ margin WRITE setMargin NOTIFY marginChanged)
    Q_PROPERTY(int pixelSize READ pixelSize WRITE setPixelSize NOTIFY pixelSizeChanged)
    Q_PROPERTY(QColor pixelColor READ pixelColor WRITE setPixelColor NOTIFY pixelColorChanged)

public:
    explicit RTTcuImageWidget(QWidget *parent = nullptr);

    // Datenübergabe: Sensorbild setzen
    void setImageData(const QVector<quint8> &data, int size);

    // Getter/Setter
    int margin() const { return m_margin; }
    void setMargin(int margin);

    int pixelSize() const { return m_pixelSize; }
    void setPixelSize(int size);

    QColor pixelColor() const { return m_pixelColor; }
    void setPixelColor(const QColor &color);

signals:
    void marginChanged(int);
    void pixelSizeChanged(int);
    void pixelColorChanged(const QColor &);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_margin;                // Innenabstand vom Rand
    int m_pixelSize;             // Größe eines Pixels
    QColor m_pixelColor;         // Basisfarbe für Pixel
    QVector<quint8> m_imageData; // Bilddaten (1D-Array)
    int m_imageSize;             // Breite/Höhe (quadratisch)
};
