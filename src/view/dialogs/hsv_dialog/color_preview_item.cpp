#include "color_preview_item.h"

ColorPreviewItem::ColorPreviewItem(int height, int width, QWidget* parent) : QWidget(parent)
{
    height_             = height;
    width_              = width;
    last_confirmed_rgb_ = rgb_t{0xFF, 0xFF, 0xFF};
    cached_rgb_         = rgb_t{0xFF, 0xFF, 0xFF};
    new_rgb_            = rgb_t{0xFF, 0xFF, 0xFF};
}

QSize ColorPreviewItem::sizeHint() const
{
    return {width_, height_};
}

void ColorPreviewItem::set_initial_color(rgb_t rgb)
{
    last_confirmed_rgb_ = rgb;
    cached_rgb_         = rgb;
    new_rgb_            = rgb;
    this->update();
}

void ColorPreviewItem::update_color(rgb_t rgb)
{
    cached_rgb_         = last_confirmed_rgb_;
    last_confirmed_rgb_ = rgb;
    new_rgb_            = rgb;
    this->update();
}

void ColorPreviewItem::update_curr_color(rgb_t rgb)
{
    new_rgb_ = rgb;
    this->update();
}

void ColorPreviewItem::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);

    painter.setPen(QColor(cached_rgb_.r, cached_rgb_.g, cached_rgb_.b));
    painter.setBrush(QColor(cached_rgb_.r, cached_rgb_.g, cached_rgb_.b));
    painter.drawRect(0, 0, width_ / 2, height_);

    painter.setPen(QColor(new_rgb_.r, new_rgb_.g, new_rgb_.b));
    painter.setBrush(QColor(new_rgb_.r, new_rgb_.g, new_rgb_.b));
    painter.drawRect(width_ / 2, 0, width_ / 2, height_);
}
