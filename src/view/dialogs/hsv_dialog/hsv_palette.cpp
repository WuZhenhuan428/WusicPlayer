#include "hsv_palette.h"
#include <algorithm>
#include <cmath>

#include <QDebug>

#define PI 3.14159265358979323846

float distance(float x, float y, float cx, float cy) {
    return sqrt(pow(x-cx, 2) + pow(y-cy, 2));
}

HSVPalette::HSVPalette(uint inner_radius, uint outer_radius, QWidget* parent)
    : QWidget(parent),
      inner_radius(inner_radius),
      outer_radius(outer_radius),
      m_hue(0.0f),
      m_saturation(0.0f),
      m_value(0.0f),
      m_cursor_rgb{0xFF, 0xFF, 0xFF},
      m_cursor_x(0),
      m_cursor_y(0),
      m_has_cursor(false),
      drag_ring_mode(false),
      drag_rect_mode(false),
      m_cache_hue_ring_valid(false),
      m_cache_rect_valid(false)
{
    this->setFixedSize(outer_radius*2, outer_radius*2);
    center = static_cast<double>(outer_radius);

    half_width = static_cast<double>(inner_radius) / sqrt(2.0);
    left = center - half_width;
    right = center + half_width;
    top = center - half_width;
    bottom = center + half_width;
}

HSVPalette::~HSVPalette() {}

QSize HSVPalette::sizeHint() const {
    return QSize(outer_radius*2, outer_radius*2);
}

hsv_t HSVPalette::getHsv() {
    if (!m_has_cursor) {
        return hsv_t{};
    }
    return hsv_t{m_hue, m_saturation, m_value};
}

rgb_t HSVPalette::getRgb() {
    if (!m_has_cursor) {
        return rgb_t{};
    }
    hsv_t hsv = {m_hue, m_saturation, m_value};
    return hsv_to_rgb(hsv);
}

bool HSVPalette::setHsv(hsv_t hsv) {
    if (hsv.h < 0.0 || hsv.h > 360.0 || hsv.s < 0.0 || hsv.s > 100.0 || hsv.v < 0.0 || hsv.v > 100.0) {
        return false;
    }

    m_hue = (hsv.h == 360.0) ? 0.0 : hsv.h;
    m_saturation = hsv.s;
    m_value = hsv.v;

    m_cursor_rgb = hsv_to_rgb(hsv);
    m_has_cursor = true;
    updateCursor();
    this->update();
    return true;
}

bool HSVPalette::setRgb(rgb_t rgb) {
    if (setHsv(rgb_to_hsv(rgb))) {
        return true;
    }
    return false;
}


void HSVPalette::updateCursor() {
    const double x = left + (m_saturation / 100.0) * (right - left);
    const double y = bottom - (m_value / 100.0) * (bottom - top);
    
    m_cursor_x = static_cast<uint>(std::lround(std::clamp(x, static_cast<double>(left), static_cast<double>(right))));
    m_cursor_y = static_cast<uint>(std::lround(std::clamp(y, static_cast<double>(top), static_cast<double>(bottom))));
}

void HSVPalette::setInnerRadius(uint radius) {
    inner_radius = radius;
}

void HSVPalette::setOuterRadius(uint radius) {
    outer_radius = radius;
}

bool HSVPalette::isInHueRing(uint x, uint y) {
    float dist = distance(x, y, outer_radius, outer_radius);
    if (dist >= inner_radius && dist <= outer_radius) {
        return true;
    }
    return false;
}

bool HSVPalette::isInRect(uint x, uint y) {
    if ((x >= (center - half_width)) && (x <= (center + half_width))) {
        if ((y >= (center - half_width)) && (y <= (center + half_width))) {
            return true;
        }
    }
    return false;
}

void HSVPalette::paintHueRing(QPainter* painter) {
    if (!m_cache_hue_ring_valid) {
        m_cache_hue_ring = QImage(outer_radius*2, outer_radius*2, QImage::Format_ARGB32_Premultiplied);
        m_cache_hue_ring.fill(Qt::transparent);
        QPainter hue_ring_painter(&m_cache_hue_ring);

        for(uint y = 0; y < outer_radius * 2; y++) {
            for (uint x = 0; x < outer_radius * 2; x++) {
                rgb_t hue_rgb;
                unsigned char alpha;
                if (isInHueRing(x, y)) {
                    float hue = coordToHue(x, y);
    
                    hsv_t hsv = {.h=hue, .s=100.0, .v=100.0};
                    hue_rgb = hsv_to_rgb(hsv);
                    alpha = 0xFF;
                } else {
                    hue_rgb.r = 0xFF;
                    hue_rgb.g = 0xFF;
                    hue_rgb.b = 0xFF;
                    alpha = 0x00;
                }
                hue_ring_painter.setPen(QColor(hue_rgb.r, hue_rgb.g, hue_rgb.b, alpha));
                hue_ring_painter.drawPoint(x, y);
            }
        }
        m_cache_hue_ring_valid = true;
        emit sgnHSVChanged({m_hue, m_saturation, m_value});
    }
    QRect target(0, 0, outer_radius*2, outer_radius*2);
    QRect source(0, 0, outer_radius*2, outer_radius*2);
    painter->drawImage(target, m_cache_hue_ring, source);
    m_cache_rect_valid = false;
}

double HSVPalette::coordToHue(uint x, uint y) {
    const float dx = static_cast<float>(x) - static_cast<float>(outer_radius);
    const float dy = static_cast<float>(y) - static_cast<float>(outer_radius);

    const float angle_r = atan2(dy, dx);
    float angle_d = angle_r * 180 / PI;
    if (angle_d <= 0.0) {
        angle_d += 360;
    }
    angle_d = 360.0 - angle_d;

    return angle_d;
}

double HSVPalette::coordToSaturation(uint x) {
    if (right <= left) return 0.0;
    double s = (static_cast<double>(x) - left) / (right - left) * 100.0;
    return std::clamp(s, 0.0, 100.0);
}

double HSVPalette::coordToValue(uint y) {
    if (bottom <= top) return 0.0;
    double v = (bottom - static_cast<double>(y)) / (bottom - top) * 100.0;
    return std::clamp(v, 0.0, 100.0);
}

void HSVPalette::paintRect(QPainter* painter) {
    if (!m_cache_rect_valid) {
        m_cache_rect_valid = true;

        m_cache_rect = QImage(outer_radius*2, outer_radius*2, QImage::Format_ARGB32_Premultiplied);
        m_cache_rect.fill(Qt::transparent);
        QPainter rect_painter(&m_cache_rect);

        for (uint x = 0; x < outer_radius * 2; x++) {
            for (uint y = 0; y < outer_radius * 2; y++) {
                if (isInRect(x, y)) {
                    hsv_t hsv;
                    hsv.h = m_hue;
                    hsv.s = coordToSaturation(x);
                    hsv.v = coordToValue(y);
                    rgb_t rect_rgb = hsv_to_rgb((hsv));
    
                    rect_painter.setPen(QColor(rect_rgb.r, rect_rgb.g, rect_rgb.b, 0xFF));
                    rect_painter.drawPoint(x, y);
                }
            }
        }
        emit sgnHSVChanged({m_hue, m_saturation, m_value});
    }
    QRect target(left, top, half_width*2, half_width*2);
    QRect source(left, top, half_width*2, half_width*2);
    painter->drawImage(target, m_cache_rect, source);
}

void HSVPalette::paintCursor(QPainter* painter) {
    if (!m_has_cursor) {
        return;
    }

    QPen pen(QColor(0xFF, 0xFF, 0xFF, 0xFF), 1);
    painter->setPen(pen);
    painter->setBrush(QColor(m_cursor_rgb.r, m_cursor_rgb.g, m_cursor_rgb.b));
    painter->drawEllipse(QPointF(m_cursor_x, m_cursor_y), 6, 6);

    QPen border(QColor(0x00, 0x00, 0x00, 0xFF), 1);
    painter->setPen(border);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(m_cursor_x, m_cursor_y), 7, 7);
}

void HSVPalette::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    this->paintRect(&painter);
    this->paintCursor(&painter);
    this->paintHueRing(&painter);
}

void HSVPalette::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (isInHueRing(x, y)) {
            m_hue = coordToHue(x, y);
            rgb_t rgb = hsv_to_rgb(hsv_t{m_hue, m_saturation, m_value});
            m_cursor_rgb = rgb;
            m_has_cursor = true;
            updateCursor();

            drag_ring_mode = true;
            drag_rect_mode = false;
        } else if (isInRect(x, y)) {
            m_saturation = coordToSaturation(x);
            m_value = coordToValue(y);
            hsv_t hsv;
            hsv.h = m_hue;
            hsv.s = m_saturation;
            hsv.v = m_value;
            rgb_t rgb = hsv_to_rgb(hsv);

            m_cursor_x = static_cast<uint>(std::clamp(static_cast<float>(x), left, right));
            m_cursor_y = static_cast<uint>(std::clamp(static_cast<float>(y), top, bottom));
            m_cursor_rgb = rgb;
            m_has_cursor = true;

            drag_ring_mode = false;
            drag_rect_mode = true;
        }
        emit sgnMouseMovingColor(m_cursor_rgb);
        this->update();
    }
    event->accept();
}

void HSVPalette::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (drag_ring_mode) {
            m_hue = coordToHue(x, y);

            rgb_t rgb = hsv_to_rgb(hsv_t{m_hue, m_saturation, m_value});
            m_cursor_rgb = rgb;
            m_has_cursor = true;

            this->update();
        }
        if (drag_rect_mode) {
            m_saturation = coordToSaturation(static_cast<uint>(x));
            m_value = coordToValue(static_cast<uint>(y));

            m_cursor_x = static_cast<uint>(std::clamp(static_cast<float>(x), left, right));
            m_cursor_y = static_cast<uint>(std::clamp(static_cast<float>(y), top, bottom));
            
            rgb_t rgb = hsv_to_rgb(hsv_t{m_hue, m_saturation, m_value});
            m_cursor_rgb = rgb;
            m_has_cursor = true;
            this->update();
        }
        emit sgnMouseMovingColor(m_cursor_rgb);
    }
    event->accept();
}

void HSVPalette::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        drag_ring_mode = false;
        drag_rect_mode = false;
        emit sgnMouseReleaseColor(m_cursor_rgb);
    }
    event->accept();
}
