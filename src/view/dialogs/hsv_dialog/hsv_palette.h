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
    explicit HSVPalette(int32_t inner_radius, int32_t outer_radius, QWidget* parent = nullptr);

    void set_inner_radius(int32_t radius);
    void set_outer_radius(int32_t radius);

    bool set_hsv(hsv_t hsv);
    bool set_rgb(rgb_t rgb);

    hsv_t get_hsv();
    rgb_t get_rgb();

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void paint_hue_ring(QPainter* painter);
    void paint_rect(QPainter* painter);
    void paint_cursor(QPainter* painter) const;

    [[nodiscard]] bool is_in_hue_ring(double x, double y) const;
    [[nodiscard]] bool is_in_rect(double x, double y) const;
    [[nodiscard]] double coord_to_hue(double x, double y) const;
    [[nodiscard]] double coord_to_saturation(double x) const;
    [[nodiscard]] double coord_to_value(double y) const;

    void update_cursor();

private:
    int32_t inner_radius_;
    int32_t outer_radius_;
    double hue_;
    double saturation_;
    double value_;
    rgb_t cursor_rgb_;
    double cursor_x_;
    double cursor_y_;
    bool has_cursor_;

    // use at mouseMoveEvent

    double center_;
    double half_width_;
    double left_;
    double right_;
    double top_;
    double bottom_;

    bool drag_ring_mode_;
    bool drag_rect_mode_;

    bool cache_hue_ring_valid_;
    QImage cache_hue_ring_;

    bool cache_rect_valid_;
    QImage cache_rect_;

signals:
    void sgn_hsv_changed(hsv_t hsv);
    void sgn_mouse_moving_color(rgb_t rgb);
    void sgn_mouse_release_color(rgb_t rgb);
};
