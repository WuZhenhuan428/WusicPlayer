#pragma once

#include "hsv_to_rgb.h"

#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

class ColorPreviewItem : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPreviewItem(int height, int width, QWidget* parent);
    ~ColorPreviewItem() = default;

    void set_initial_color(rgb_t rgb);
    void update_color(rgb_t rgb);
    void update_curr_color(rgb_t rgb);

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override;

private:
    int m_height;
    int m_width;
    rgb_t m_last_confirmed_rgb;
    rgb_t m_cached_rgb;
    rgb_t m_new_rgb;
};
