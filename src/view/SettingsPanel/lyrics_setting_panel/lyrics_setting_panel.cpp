#include "lyrics_setting_panel.h"

#include "view/dialogs/font_select_dialog.h"
#include "view/dialogs/hsv_dialog/hsv_dialog.h"

#include <QFont>
#include <QPalette>

LyricsSettingPanel::LyricsSettingPanel(QWidget* parent)
    : QWidget(parent)
{
    m_is_two_line = false;
    init_UI();
    init_connections();
}

QListWidgetItem* LyricsSettingPanel::getTitleItem() {
    if (!m_title_widget) {
        m_title_widget = new QListWidgetItem("Lyrics");
    }
    return m_title_widget;
}

void LyricsSettingPanel::init_UI() {
    // font line
    m_lb_font = new QLabel("Font: ", this);
    m_le_font = new QLineEdit(this);
    m_btn_font = new QPushButton("Edit", this);
    m_hbl_font = new QHBoxLayout;
    m_hbl_font->addWidget(m_lb_font);
    m_hbl_font->addWidget(m_le_font);
    m_hbl_font->addWidget(m_btn_font);

    // line mode
    m_lb_mode = new QLabel("Display Mode: ", this);
    m_rb_oneline = new QRadioButton("One Line", this);
    m_rb_twoline = new QRadioButton("two Line", this);
    m_hbl_mode = new QHBoxLayout;
    m_hbl_mode->addWidget(m_lb_mode);
    m_hbl_mode->addWidget(m_rb_oneline);
    m_hbl_mode->addWidget(m_rb_twoline);
    m_hbl_mode->addStretch();
    /// logic:
    m_mode_group = new QButtonGroup(this);
    m_mode_group->addButton(m_rb_oneline);
    m_mode_group->addButton(m_rb_twoline);
    m_mode_group->setExclusive(true);

    // color
    m_lb_color = new QLabel("Color: ",this);
    m_lb_active = new QLabel("Active color: ", this);
    m_lb_inactive = new QLabel("Inactive color: ", this);
    m_btn_active_color = new QPushButton(this);
    m_btn_inactive_color = new QPushButton(this);
    m_hbl_color = new QHBoxLayout;
    m_hbl_color->addWidget(m_lb_color);
    m_hbl_color->addWidget(m_lb_active);
    m_hbl_color->addWidget(m_btn_active_color);
    m_hbl_color->addWidget(m_lb_inactive);
    m_hbl_color->addWidget(m_btn_inactive_color);

    // all
    m_vbl_main = new QVBoxLayout;
    m_vbl_main->addLayout(m_hbl_font);
    m_vbl_main->addLayout(m_hbl_mode);
    m_vbl_main->addLayout(m_hbl_color);

    this->setLayout(m_vbl_main);

    if (!m_title_widget) {
        m_title_widget = new QListWidgetItem("Lyrics");
    }
    
    update_active_btn_color(m_active_rgb);
    update_inactive_btn_color(m_inactive_rgb);
}

void LyricsSettingPanel::update_active_btn_color(rgb_t rgb) {
    QPalette pal = m_btn_active_color->palette();
    pal.setColor(QPalette::Button, QColor(rgb.r, rgb.g, rgb.b));
    m_btn_active_color->setAutoFillBackground(true);
    m_btn_active_color->setPalette(pal);
}

void LyricsSettingPanel::update_inactive_btn_color(rgb_t rgb) {
    QPalette pal = m_btn_inactive_color->palette();
    pal.setColor(QPalette::Button, QColor(rgb.r, rgb.g, rgb.b));
    m_btn_inactive_color->setAutoFillBackground(true);
    m_btn_inactive_color->setPalette(pal);
}

void LyricsSettingPanel::init_connections() {
    connect(m_btn_font, &QPushButton::clicked, this, [this](){
        FontSelectDialog font_dialog = FontSelectDialog(m_font, this);
        int result = font_dialog.exec();
        if (result == QDialog::Accepted) {
            m_font = font_dialog.selectFont();
            QString template_str = "%1 %2 %3pt";
            m_font_view = template_str.arg(m_font.family()).arg(m_font.styleName()).arg(m_font.pointSize());

            m_le_font->setText(m_font_view);
        }
    });

    connect(m_rb_oneline, &QRadioButton::toggled, this, [this](bool is_checked){
        if (is_checked) {
            m_is_two_line = false;
            emit sgnDisplayModeChanged(m_is_two_line);
        }
    });
    connect(m_rb_twoline, &QRadioButton::toggled, this, [this](bool is_checked){
        if (is_checked) {
            m_is_two_line = true;
            emit sgnDisplayModeChanged(m_is_two_line);
        }
    });

    connect(m_btn_active_color, &QPushButton::clicked, this, [this](){
        HSVDialog hsv_panel(rgb_t{0x00, 0x00, 0x00});
        int result = hsv_panel.exec();
        if (result == QDialog::Accepted) {
            m_active_rgb = hsv_panel.getColor();
            update_active_btn_color(m_active_rgb);
            emit sgnActiveColorChanged(m_active_rgb);
        }
    });

    connect(m_btn_inactive_color, &QPushButton::clicked, this, [this](){
        HSVDialog hsv_panel(rgb_t{0x00, 0x00, 0x00});
        int result = hsv_panel.exec();
        if (result == QDialog::Accepted) {
            m_inactive_rgb = hsv_panel.getColor();
            update_inactive_btn_color(m_inactive_rgb);
            emit sgnInactiveColorChanged(m_inactive_rgb);
        }
    });
}
