#pragma once

#include <QWidget>
#include <QObject>
#include <QMouseEvent>
#include <QPainter>
#include "hsv_to_rgb.h"
#include <QString>
#include <QImage>

class HSVPalette : public QWidget
{
    Q_OBJECT
public:
    explicit HSVPalette(uint inner_radius, uint outer_radius, QWidget* parent = nullptr);
    ~HSVPalette();

    void setInnerRadius(uint radius);
    void setOuterRadius(uint radius);
    
    bool setHsv(hsv_t hsv);
    bool setRgb(rgb_t rgb);
    
    hsv_t getHsv();
    rgb_t getRgb();

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void paintHueRing(QPainter* painter);
    void paintRect(QPainter* painter);
    void paintCursor(QPainter* painter);

    bool isInHueRing(uint x, uint y);
    bool isInRect(uint x, uint y);
    double coordToHue(uint x, uint y);
    double coordToSaturation(uint x);
    double coordToValue(uint y);

    void updateCursor();

private:
    uint inner_radius;
    uint outer_radius;
    float m_hue;
    float m_saturation;
    float m_value;
    rgb_t m_cursor_rgb;
    uint m_cursor_x;
    uint m_cursor_y;
    bool m_has_cursor;
    
    // use at mouseMoveEvent
    
    float center;
    float half_width;
    float left;
    float right;
    float top;
    float bottom;

    bool drag_ring_mode;
    bool drag_rect_mode;

    bool m_cache_hue_ring_valid;
    QImage m_cache_hue_ring;

    float m_cache_rect_valid;
    QImage m_cache_rect;

signals:
    void sgnHSVChanged(hsv_t hsv);
    void sgnMouseMovingColor(rgb_t rgb);
    void sgnMouseReleaseColor(rgb_t rgb);
};