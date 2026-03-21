#pragma once

#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QPaintEvent>

#include "hsv_to_rgb.h"

class ColorPreviewItem : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPreviewItem(int height, int width, QWidget* parent);
    ~ColorPreviewItem() = default;

    void setInitialColor(rgb_t rgb);
    void updateColor(rgb_t rgb);
    void updateCurrColor(rgb_t rgb);

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