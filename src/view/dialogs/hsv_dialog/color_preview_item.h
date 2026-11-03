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

    void set_initial_color(rgb_t rgb);
    void update_color(rgb_t rgb);
    void update_curr_color(rgb_t rgb);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int height_;
    int width_;
    rgb_t last_confirmed_rgb_ = {};
    rgb_t cached_rgb_         = {};
    rgb_t new_rgb_            = {};
};
