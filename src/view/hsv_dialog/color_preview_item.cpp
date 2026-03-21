#include "color_preview_item.h"

ColorPreviewItem::ColorPreviewItem(int height, int width, QWidget* parent)
    : QWidget(parent)
{
    m_height = height;
    m_width = width;
    m_last_confirmed_rgb = rgb_t{0xFF, 0xFF, 0xFF};
    m_cached_rgb = rgb_t{0xFF, 0xFF, 0xFF};
    m_new_rgb = rgb_t{0xFF, 0xFF, 0xFF};
}

QSize ColorPreviewItem::sizeHint() const {
    return QSize(m_width, m_height);
}

void ColorPreviewItem::setInitialColor(rgb_t rgb) {
    m_last_confirmed_rgb = rgb;
    m_cached_rgb = rgb;
    m_new_rgb = rgb;
    this->update();
}

void ColorPreviewItem::updateColor(rgb_t rgb) {
    m_cached_rgb = m_last_confirmed_rgb;
    m_last_confirmed_rgb = rgb;
    m_new_rgb = rgb;
    this->update();
}

void ColorPreviewItem::updateCurrColor(rgb_t rgb) {
    m_new_rgb = rgb;
    this->update();
}

void ColorPreviewItem::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    painter.setPen(QColor(m_cached_rgb.r, m_cached_rgb.g, m_cached_rgb.b));
    painter.setBrush(QColor(m_cached_rgb.r, m_cached_rgb.g, m_cached_rgb.b));
    painter.drawRect(0, 0, m_width/2, m_height);

    painter.setPen(QColor(m_new_rgb.r, m_new_rgb.g, m_new_rgb.b));
    painter.setBrush(QColor(m_new_rgb.r, m_new_rgb.g, m_new_rgb.b));
    painter.drawRect(m_width/2, 0, m_width/2, m_height);
}
