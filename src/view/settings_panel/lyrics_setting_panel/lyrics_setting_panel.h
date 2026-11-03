#pragma once

#include "core/hsv_types.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWidget>

class LyricsSettingPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LyricsSettingPanel(rgb_t active, rgb_t inactive, QWidget* parent = nullptr);

    QListWidgetItem* get_title_item();
    void set_line_edit_text(const QString& font_string);
    void set_line_edit_text(const QFont& font);

private:
    QFont font_;
    QString font_view_;

    bool is_two_line_;

    rgb_t active_rgb_   = rgb_t{0xFF, 0xFF, 0xFF};
    rgb_t inactive_rgb_ = rgb_t{0x00, 0x00, 0x00};

    void update_active_btn_color(rgb_t rgb);
    void update_inactive_btn_color(rgb_t rgb);

    void init_ui();
    void init_connections();

    static QString make_font_view(const QFont& font);
    static QString make_font_view(const QString& raw_string);

signals:
    void sgn_font_changed(QFont font);
    void sgn_display_mode_changed(bool is_two_line);
    void sgn_active_color_changed(rgb_t rgb);
    void sgn_inactive_color_changed(rgb_t rgb);

private:
    // font
    QLabel* lb_font_                 = nullptr;
    QLineEdit* le_font_              = nullptr;
    QPushButton* btn_font_           = nullptr;
    QHBoxLayout* hbl_font_           = nullptr;

    // line select
    QLabel* lb_mode_                 = nullptr;
    QRadioButton* rb_oneline_        = nullptr;
    QRadioButton* rb_twoline_        = nullptr;
    QHBoxLayout* hbl_mode_           = nullptr;
    QButtonGroup* mode_group_        = nullptr;

    // color select
    QLabel* lb_color_                = nullptr;
    QLabel* lb_active_               = nullptr;
    QLabel* lb_inactive_             = nullptr;
    QPushButton* btn_active_color_   = nullptr;
    QPushButton* btn_inactive_color_ = nullptr;
    QHBoxLayout* hbl_color_          = nullptr;

    QVBoxLayout* vbl_main_           = nullptr;

    QListWidgetItem* title_widget_   = nullptr;
};
