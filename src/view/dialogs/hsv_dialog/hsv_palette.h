#pragma once

#include "hsv_to_rgb.h"

#include <QImage>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QString>
#include <QWidget>

class HSVPalette : public QWidget
{
    Q_OBJECT
public:
    explicit HSVPalette(uint inner_radius, uint outer_radius, QWidget* parent = nullptr);
    ~HSVPalette();

    void set_inner_radius(uint radius);
    void set_outer_radius(uint radius);

    bool set_hsv(hsv_t hsv);
    bool set_rgb(rgb_t rgb);

    hsv_t get_hsv();
    rgb_t get_rgb();

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void paint_hue_ring(QPainter* painter);
    void paint_rect(QPainter* painter);
    void paint_cursor(QPainter* painter);

    bool is_in_hue_ring(uint x, uint y);
    bool is_in_rect(uint x, uint y);
    double coord_to_hue(uint x, uint y);
    double coord_to_saturation(uint x);
    double coord_to_value(uint y);

    void update_cursor();

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
