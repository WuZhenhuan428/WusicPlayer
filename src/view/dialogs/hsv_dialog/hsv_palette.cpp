#include "hsv_palette.h"
#include <algorithm>
#include <numbers>

namespace
{
double distance(double x, double y, double cx, double cy)
{
    return sqrt(pow(x - cx, 2) + pow(y - cy, 2));
}
} // namespace

HSVPalette::HSVPalette(int32_t inner_radius, int32_t outer_radius, QWidget* parent) :
    QWidget(parent), inner_radius_(inner_radius), outer_radius_(outer_radius), hue_(0.0f),
    saturation_(0.0f), value_(0.0f), cursor_rgb_{0xFF, 0xFF, 0xFF}, cursor_x_(0), cursor_y_(0),
    has_cursor_(false), drag_ring_mode_(false), drag_rect_mode_(false),
    cache_hue_ring_valid_(false), cache_rect_valid_(false)
{
    this->setFixedSize(outer_radius * 2, outer_radius * 2);
    center_     = static_cast<double>(outer_radius);

    half_width_ = static_cast<double>(inner_radius) / std::numbers::sqrt2;
    left_       = center_ - half_width_;
    right_      = center_ + half_width_;
    top_        = center_ - half_width_;
    bottom_     = center_ + half_width_;
}

QSize HSVPalette::sizeHint() const
{
    return {outer_radius_ * 2, outer_radius_ * 2};
}

hsv_t HSVPalette::get_hsv()
{
    if (!has_cursor_) {
        return hsv_t{};
    }
    return hsv_t{hue_, saturation_, value_};
}

rgb_t HSVPalette::get_rgb()
{
    if (!has_cursor_) {
        return rgb_t{};
    }
    hsv_t hsv = {hue_, saturation_, value_};
    return hsv_to_rgb(hsv);
}

bool HSVPalette::set_hsv(hsv_t hsv)
{
    if (hsv.h < 0.0 || hsv.h > 360.0 || hsv.s < 0.0 || hsv.s > 100.0 || hsv.v < 0.0 ||
        hsv.v > 100.0) {
        return false;
    }

    hue_        = (hsv.h == 360.0) ? 0.0 : hsv.h;
    saturation_ = hsv.s;
    value_      = hsv.v;

    cursor_rgb_ = hsv_to_rgb(hsv);
    has_cursor_ = true;
    update_cursor();
    this->update();
    return true;
}

bool HSVPalette::set_rgb(rgb_t rgb)
{
    if (set_hsv(rgb_to_hsv(rgb))) {
        return true;
    }
    return false;
}

void HSVPalette::update_cursor()
{
    const double x = left_ + ((saturation_ / 100.0) * (right_ - left_));
    const double y = bottom_ - ((value_ / 100.0) * (bottom_ - top_));

    cursor_x_      = static_cast<double>(std::lround(std::clamp(x, left_, right_)));
    cursor_y_      = static_cast<double>(std::lround(std::clamp(y, top_, bottom_)));
}

void HSVPalette::set_inner_radius(int32_t radius)
{
    inner_radius_ = radius;
}

void HSVPalette::set_outer_radius(int32_t radius)
{
    outer_radius_ = radius;
}

bool HSVPalette::is_in_hue_ring(double x, double y) const
{
    double dist = distance(x, y, outer_radius_, outer_radius_);
    if (dist >= inner_radius_ && dist <= outer_radius_) {
        return true;
    }
    return false;
}

bool HSVPalette::is_in_rect(double x, double y) const
{
    if ((x >= (center_ - half_width_)) && (x <= (center_ + half_width_))) {
        if ((y >= (center_ - half_width_)) && (y <= (center_ + half_width_))) {
            return true;
        }
    }
    return false;
}

void HSVPalette::paint_hue_ring(QPainter* painter)
{
    if (!cache_hue_ring_valid_) {
        cache_hue_ring_ =
            QImage(static_cast<int>(outer_radius_) * 2, static_cast<int>(outer_radius_) * 2,
                   QImage::Format_ARGB32_Premultiplied);
        cache_hue_ring_.fill(Qt::transparent);
        QPainter hue_ring_painter(&cache_hue_ring_);

        for (int32_t y = 0; y < outer_radius_ * 2; y++) {
            for (int32_t x = 0; x < outer_radius_ * 2; x++) {
                rgb_t hue_rgb;
                unsigned char alpha = 0;
                if (is_in_hue_ring(x, y)) {
                    double hue = coord_to_hue(x, y);

                    hsv_t hsv  = {.h = hue, .s = 100.0, .v = 100.0};
                    hue_rgb    = hsv_to_rgb(hsv);
                    alpha      = 0xFF;
                } else {
                    hue_rgb.r = 0xFF;
                    hue_rgb.g = 0xFF;
                    hue_rgb.b = 0xFF;
                    alpha     = 0x00;
                }
                hue_ring_painter.setPen(QColor(hue_rgb.r, hue_rgb.g, hue_rgb.b, alpha));
                hue_ring_painter.drawPoint(x, y);
            }
        }
        cache_hue_ring_valid_ = true;
        emit sgn_hsv_changed({hue_, saturation_, value_});
    }
    QRect target(0, 0, outer_radius_ * 2, outer_radius_ * 2);
    QRect source(0, 0, outer_radius_ * 2, outer_radius_ * 2);
    painter->drawImage(target, cache_hue_ring_, source);
    cache_rect_valid_ = false;
}

double HSVPalette::coord_to_hue(double x, double y) const
{
    const double dx      = x - outer_radius_;
    const double dy      = y - outer_radius_;

    const double angle_r = std::atan2(dy, dx);
    double angle_d       = angle_r * 180 / std::numbers::pi_v<double>;
    if (angle_d <= 0.0) {
        angle_d += 360;
    }
    angle_d = 360.0f - angle_d;

    return angle_d;
}

double HSVPalette::coord_to_saturation(double x) const
{
    if (right_ <= left_) {
        return 0.0;
    }
    double s = (x - left_) / (right_ - left_) * 100.0;
    return std::clamp(s, 0.0, 100.0);
}

double HSVPalette::coord_to_value(double y) const
{
    if (bottom_ <= top_) {
        return 0.0;
    }
    double v = (bottom_ - y) / (bottom_ - top_) * 100.0;
    return std::clamp(v, 0.0, 100.0);
}

void HSVPalette::paint_rect(QPainter* painter)
{
    if (!cache_rect_valid_) {
        cache_rect_valid_ = true;

        cache_rect_ =
            QImage(outer_radius_ * 2, outer_radius_ * 2, QImage::Format_ARGB32_Premultiplied);
        cache_rect_.fill(Qt::transparent);
        QPainter rect_painter(&cache_rect_);

        for (int32_t x = 0; x < outer_radius_ * 2; x++) {
            for (int32_t y = 0; y < outer_radius_ * 2; y++) {
                if (is_in_rect(x, y)) {
                    hsv_t hsv;
                    hsv.h          = hue_;
                    hsv.s          = coord_to_saturation(x);
                    hsv.v          = coord_to_value(y);
                    rgb_t rect_rgb = hsv_to_rgb(hsv);

                    rect_painter.setPen(QColor(rect_rgb.r, rect_rgb.g, rect_rgb.b, 0xFF));
                    rect_painter.drawPoint(x, y);
                }
            }
        }
        emit sgn_hsv_changed({hue_, saturation_, value_});
    }
    QRect target(static_cast<int>(left_), static_cast<int>(top_), static_cast<int>(half_width_) * 2,
                 static_cast<int>(half_width_) * 2);
    QRect source(static_cast<int>(left_), static_cast<int>(top_), static_cast<int>(half_width_) * 2,
                 static_cast<int>(half_width_) * 2);
    painter->drawImage(target, cache_rect_, source);
}

void HSVPalette::paint_cursor(QPainter* painter) const
{
    if (!has_cursor_) {
        return;
    }

    QPen pen(QColor(0xFF, 0xFF, 0xFF, 0xFF), 1);
    painter->setPen(pen);
    painter->setBrush(QColor(cursor_rgb_.r, cursor_rgb_.g, cursor_rgb_.b));
    painter->drawEllipse(QPointF(cursor_x_, cursor_y_), 6, 6);

    QPen border(QColor(0x00, 0x00, 0x00, 0xFF), 1);
    painter->setPen(border);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(cursor_x_, cursor_y_), 7, 7);
}

void HSVPalette::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);

    this->paint_rect(&painter);
    this->paint_cursor(&painter);
    this->paint_hue_ring(&painter);
}

void HSVPalette::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (is_in_hue_ring(x, y)) {
            hue_        = coord_to_hue(x, y);
            rgb_t rgb   = hsv_to_rgb(hsv_t{hue_, saturation_, value_});
            cursor_rgb_ = rgb;
            has_cursor_ = true;
            update_cursor();

            drag_ring_mode_ = true;
            drag_rect_mode_ = false;
        } else if (is_in_rect(x, y)) {
            saturation_ = coord_to_saturation(x);
            value_      = coord_to_value(y);
            hsv_t hsv;
            hsv.h       = hue_;
            hsv.s       = saturation_;
            hsv.v       = value_;
            rgb_t rgb   = hsv_to_rgb(hsv);

            cursor_x_   = static_cast<double>(std::clamp(static_cast<double>(x), left_, right_));
            cursor_y_   = static_cast<double>(std::clamp(static_cast<double>(y), top_, bottom_));
            cursor_rgb_ = rgb;
            has_cursor_ = true;

            drag_ring_mode_ = false;
            drag_rect_mode_ = true;
        }
        emit sgn_mouse_moving_color(cursor_rgb_);
        this->update();
    }
    event->accept();
}

void HSVPalette::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (drag_ring_mode_) {
            hue_        = coord_to_hue(x, y);

            rgb_t rgb   = hsv_to_rgb(hsv_t{hue_, saturation_, value_});
            cursor_rgb_ = rgb;
            has_cursor_ = true;

            this->update();
        }
        if (drag_rect_mode_) {
            saturation_ = coord_to_saturation(static_cast<double>(x));
            value_      = coord_to_value(static_cast<double>(y));

            cursor_x_   = static_cast<double>(std::clamp(static_cast<double>(x), left_, right_));
            cursor_y_   = static_cast<double>(std::clamp(static_cast<double>(y), top_, bottom_));

            rgb_t rgb   = hsv_to_rgb(hsv_t{hue_, saturation_, value_});
            cursor_rgb_ = rgb;
            has_cursor_ = true;
            this->update();
        }
        emit sgn_mouse_moving_color(cursor_rgb_);
    }
    event->accept();
}

void HSVPalette::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        drag_ring_mode_ = false;
        drag_rect_mode_ = false;
        emit sgn_mouse_release_color(cursor_rgb_);
    }
    event->accept();
}
