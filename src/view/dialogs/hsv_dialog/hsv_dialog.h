#pragma once

#include "view/dialogs/hsv_dialog/color_preview_item.h"
#include "view/dialogs/hsv_dialog/hsv_palette.h"
#include "view/dialogs/hsv_dialog/label_matrix.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class HSVDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HSVDialog(rgb_t curr_rgb, QWidget* parent = nullptr);

    rgb_t get_color();

private:
    rgb_t current_color_ = {};

    HSVPalette* hsv_palette_;
    ColorPreviewItem* color_preview_;
    LabelMatrix* lb_matrix_;
    QVBoxLayout* vbl_color_;
    QHBoxLayout* hbl_palette_;

    QPushButton* btn_apply_;
    QPushButton* btn_ok_;
    QPushButton* btn_cancel_;
    QHBoxLayout* bottom_layout_;
    QVBoxLayout* main_layout_;

signals:
    void sgn_select_color(rgb_t rgb);
};
