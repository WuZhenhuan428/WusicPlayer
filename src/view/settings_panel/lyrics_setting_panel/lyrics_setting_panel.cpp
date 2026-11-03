#include "view/settings_panel/lyrics_setting_panel/lyrics_setting_panel.h"

#include "view/dialogs/font_select_dialog.h"
#include "view/dialogs/hsv_dialog/hsv_dialog.h"

#include <QFont>
#include <QPalette>
#include <qfont.h>

LyricsSettingPanel::LyricsSettingPanel(rgb_t active, rgb_t inactive, QWidget* parent) :
    QWidget(parent)
{
    is_two_line_  = false;
    active_rgb_   = active;
    inactive_rgb_ = inactive;
    init_ui();
    init_connections();
}

QListWidgetItem* LyricsSettingPanel::get_title_item()
{
    if (!title_widget_) {
        title_widget_ = new QListWidgetItem(tr("Lyrics"));
    }
    return title_widget_;
}

void LyricsSettingPanel::set_line_edit_text(const QString& font_string)
{
    le_font_->setText(make_font_view(font_string));
}

void LyricsSettingPanel::set_line_edit_text(const QFont& font)
{
    le_font_->setText(make_font_view(font));
}

QString LyricsSettingPanel::make_font_view(const QFont& font)
{
    QString template_str = "%1 %2 %3pt";
    return template_str.arg(font.family()).arg(font.styleName()).arg(font.pointSize());
}

QString LyricsSettingPanel::make_font_view(const QString& raw_string)
{
    QFont font;
    font.fromString(raw_string);
    return make_font_view(font);
}

void LyricsSettingPanel::init_ui()
{
    // font line
    lb_font_ = new QLabel(tr("Font: "), this);
    le_font_ = new QLineEdit(this);
    le_font_->setReadOnly(true);
    btn_font_ = new QPushButton(tr("Edit"), this);
    hbl_font_ = new QHBoxLayout;
    hbl_font_->addWidget(lb_font_);
    hbl_font_->addWidget(le_font_);
    hbl_font_->addWidget(btn_font_);

    // line mode
    lb_mode_    = new QLabel(tr("Display Mode: "), this);
    rb_oneline_ = new QRadioButton(tr("One Line"), this);
    rb_twoline_ = new QRadioButton(tr("two Line"), this);
    hbl_mode_   = new QHBoxLayout;
    hbl_mode_->addWidget(lb_mode_);
    hbl_mode_->addWidget(rb_oneline_);
    hbl_mode_->addWidget(rb_twoline_);
    hbl_mode_->addStretch();
    /// logic:
    mode_group_ = new QButtonGroup(this);
    mode_group_->addButton(rb_oneline_);
    mode_group_->addButton(rb_twoline_);
    mode_group_->setExclusive(true);
    rb_oneline_->setChecked(true);

    // color
    lb_color_           = new QLabel(tr("Color: "), this);
    lb_active_          = new QLabel(tr("Active color: "), this);
    lb_inactive_        = new QLabel(tr("Inactive color: "), this);
    btn_active_color_   = new QPushButton(this);
    btn_inactive_color_ = new QPushButton(this);
    hbl_color_          = new QHBoxLayout;
    hbl_color_->addWidget(lb_color_);
    hbl_color_->addWidget(lb_active_);
    hbl_color_->addWidget(btn_active_color_);
    hbl_color_->addWidget(lb_inactive_);
    hbl_color_->addWidget(btn_inactive_color_);
    hbl_color_->addStretch();

    // all
    vbl_main_ = new QVBoxLayout;
    vbl_main_->addLayout(hbl_font_);
    vbl_main_->addLayout(hbl_mode_);
    vbl_main_->addLayout(hbl_color_);
    vbl_main_->addStretch();

    this->setLayout(vbl_main_);

    if (!title_widget_) {
        title_widget_ = new QListWidgetItem(tr("Lyrics"));
    }

    update_active_btn_color(active_rgb_);
    update_inactive_btn_color(inactive_rgb_);
}

void LyricsSettingPanel::update_active_btn_color(rgb_t rgb)
{
    QPalette pal = btn_active_color_->palette();
    pal.setColor(QPalette::Button, QColor(rgb.r, rgb.g, rgb.b));
    btn_active_color_->setAutoFillBackground(true);
    btn_active_color_->setPalette(pal);
}

void LyricsSettingPanel::update_inactive_btn_color(rgb_t rgb)
{
    QPalette pal = btn_inactive_color_->palette();
    pal.setColor(QPalette::Button, QColor(rgb.r, rgb.g, rgb.b));
    btn_inactive_color_->setAutoFillBackground(true);
    btn_inactive_color_->setPalette(pal);
}

void LyricsSettingPanel::init_connections()
{
    connect(btn_font_, &QPushButton::clicked, this, [this]() {
        FontSelectDialog font_dialog = FontSelectDialog(font_, this);
        int result                   = font_dialog.exec();
        if (result == QDialog::Accepted) {
            font_      = font_dialog.select_font();
            font_view_ = make_font_view(font_);

            le_font_->setText(font_view_);
            emit sgn_font_changed(font_);
        }
    });

    connect(rb_oneline_, &QRadioButton::toggled, this, [this](bool is_checked) {
        if (is_checked) {
            is_two_line_ = false;
            emit sgn_display_mode_changed(is_two_line_);
        }
    });
    connect(rb_twoline_, &QRadioButton::toggled, this, [this](bool is_checked) {
        if (is_checked) {
            is_two_line_ = true;
            emit sgn_display_mode_changed(is_two_line_);
        }
    });

    connect(btn_active_color_, &QPushButton::clicked, this, [this]() {
        HSVDialog hsv_panel(rgb_t{0x00, 0x00, 0x00});
        int result = hsv_panel.exec();
        if (result == QDialog::Accepted) {
            active_rgb_ = hsv_panel.get_color();
            update_active_btn_color(active_rgb_);
            emit sgn_active_color_changed(active_rgb_);
        }
    });

    connect(btn_inactive_color_, &QPushButton::clicked, this, [this]() {
        HSVDialog hsv_panel(rgb_t{0x00, 0x00, 0x00});
        int result = hsv_panel.exec();
        if (result == QDialog::Accepted) {
            inactive_rgb_ = hsv_panel.get_color();
            update_inactive_btn_color(inactive_rgb_);
            emit sgn_inactive_color_changed(inactive_rgb_);
        }
    });
}
