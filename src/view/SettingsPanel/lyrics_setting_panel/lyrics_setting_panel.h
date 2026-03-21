#pragma once

#include <QWidget>
#include <QListWidgetItem>

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>     // n->1 button

#include "core/hsv_types.h"


class LyricsSettingPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LyricsSettingPanel(rgb_t active, rgb_t inactive, QWidget* parent = nullptr);
    ~LyricsSettingPanel() = default;

    QListWidgetItem* getTitleItem();

private:
    QFont m_font;
    QString m_font_view;

    bool m_is_two_line;

    rgb_t m_active_rgb = rgb_t{0xFF, 0xFF, 0xFF};
    rgb_t m_inactive_rgb = rgb_t{0x00, 0x00, 0x00};

    void update_active_btn_color(rgb_t rgb);
    void update_inactive_btn_color(rgb_t rgb);

    void init_UI();
    void init_connections();

signals:
    void sgnFontChanged(QFont font);
    void sgnDisplayModeChanged(bool is_two_line);
    void sgnActiveColorChanged(rgb_t rgb);
    void sgnInactiveColorChanged(rgb_t rgb);

private:
    // font
    QLabel* m_lb_font;
    QLineEdit* m_le_font;
    QPushButton* m_btn_font;
    QHBoxLayout* m_hbl_font;

    // line select
    QLabel* m_lb_mode;
    QRadioButton* m_rb_oneline;
    QRadioButton* m_rb_twoline;
    QHBoxLayout* m_hbl_mode;
    QButtonGroup* m_mode_group;

    // color select
    QLabel* m_lb_color;
    QLabel* m_lb_active;
    QLabel* m_lb_inactive;
    QPushButton* m_btn_active_color;
    QPushButton* m_btn_inactive_color;
    QHBoxLayout* m_hbl_color;

    QVBoxLayout* m_vbl_main;

    QListWidgetItem* m_title_widget = nullptr;
};