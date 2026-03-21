#pragma once

#include "hsv_palette.h"
#include "label_matrix.h"
#include "color_preview_item.h"

#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>


class HSVDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HSVDialog(rgb_t curr_rgb, QWidget *parent = nullptr);
    ~HSVDialog();

    rgb_t getColor();

private:
    rgb_t m_current_color;

    HSVPalette* m_hsv_palette;
    ColorPreviewItem* m_color_preview;
    LabelMatrix* m_label_matrix;
    QVBoxLayout* m_color_layout;
    QHBoxLayout* m_palette_layout;

    QPushButton* m_btn_apply;
    QPushButton* m_btn_ok;
    QPushButton* m_btn_cancel;
    QHBoxLayout* m_bottom_layout;
    QVBoxLayout* m_main_layout;

signals:
    void sgnSelectColor(rgb_t rgb);
};
