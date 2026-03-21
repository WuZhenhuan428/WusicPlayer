#include "font_select_dialog.h"

#include <QDebug>
#include <algorithm>
#include <QStyle>

FontSelectDialog::FontSelectDialog(QFont default_font, QWidget* parent)
    : QDialog(parent)
{
    init_UI();
    init_connections();
    init_fonts();
    m_family = default_font.family();
    m_style = default_font.styleName();
    m_size = default_font.pointSize();
    this->update_styles(m_family);
    this->update_sizes(m_family);
    this->update_preview();
    this->setMinimumSize(800, 500);
}

QFont FontSelectDialog::selectFont() {
    QFont font = QFontDatabase::font(m_family, m_style, m_size);
    return font;
}

void FontSelectDialog::init_fonts() {
    m_fonts = QFontDatabase::families(QFontDatabase::Any);
    m_list_font->addItems(m_fonts);
    const int family_index = m_fonts.indexOf(m_family);
    if (family_index >= 0) {
        m_list_font->setCurrentRow(family_index);
    }
    m_list_font->update();
}

void FontSelectDialog::init_connections() {
    connect(m_list_font, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        m_family = item->text();
        this->update_styles(m_family);
        this->update_sizes(m_family);
        this->update_preview();
    });

    connect(m_list_style, &QListWidget::itemClicked, this, [this](QListWidgetItem * item){
        m_style = item->text();
        this->update_preview();
    });

    connect(m_list_size, &QListWidget::itemClicked, this, [this](QListWidgetItem * item){
        m_size = item->text().toInt();
        m_spin_box_size->setValue(m_size);
        this->update_preview();
    });

    connect(m_spin_box_size, &QSpinBox::valueChanged, this, [this](int value){
        m_size = value;
        this->update_preview();
    });

    connect(m_btn_ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_btn_cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void FontSelectDialog::update_styles(const QString& family) {
    QStringList styles = QFontDatabase::styles(family);
    m_list_style->clear();
    m_list_style->addItems(styles);
    const int style_index = styles.indexOf(m_style);
    if (style_index >= 0) {
        m_list_style->setCurrentRow(style_index);
    } else if (!styles.isEmpty()) {
        m_list_style->setCurrentRow(0);
        m_style = styles.first();
    }
    m_list_style->update();
}

void FontSelectDialog::update_sizes(const QString& family) {
    QList<int> sizes = QFontDatabase::pointSizes(family);
    m_list_size->clear();

    if (sizes.isEmpty()) {
        m_spin_box_size->setRange(1, 999);
        m_spin_box_size->setValue(m_size);
        m_list_size->update();
        return;
    }

    int min = sizes.first();
    int max = sizes.first();
    for (int s : sizes) {
        if ( s < min) { min = s; }
        if ( s > max) { max = s; }
        m_list_size->addItem(QString::number(s));
    }
    int valid_value = std::clamp(m_size, min, max);
    m_size = valid_value;
    m_spin_box_size->setValue(valid_value);
    m_spin_box_size->setRange(min, max);

    int size_row = -1;
    for (int i = 0; i < sizes.size(); ++i) {
        if (sizes.at(i) == valid_value) {
            size_row = i;
            break;
        }
    }
    if (size_row >= 0) {
        m_list_size->setCurrentRow(size_row);
    }
    m_list_size->update();
}

void FontSelectDialog::update_preview() {
    QFont font = QFontDatabase::font(m_family, m_style, m_size);
    m_te_preview->setFont(font);
}

void FontSelectDialog::init_UI() {
    m_lb_font = new QLabel("Fonts:", this);
    m_list_font = new QListWidget(this);
    m_vbl_font = new QVBoxLayout;

    m_vbl_font->addWidget(m_lb_font);
    m_vbl_font->addWidget(m_list_font);


    m_lb_style = new QLabel("Style:", this);
    m_list_style = new QListWidget(this);
    m_vbl_style = new QVBoxLayout;
    m_vbl_style->addWidget(m_lb_style);
    m_vbl_style->addWidget(m_list_style);


    m_lb_size = new QLabel("Size:", this);
    m_spin_box_size = new QSpinBox(this);
    m_list_size = new QListWidget(this);
    m_vbl_size = new QVBoxLayout;
    m_vbl_size->addWidget(m_lb_size);
    m_vbl_size->addWidget(m_spin_box_size);
    m_vbl_size->addWidget(m_list_size);

    m_hbl_font_all = new QHBoxLayout;
    m_hbl_font_all->addLayout(m_vbl_font);
    m_hbl_font_all->addLayout(m_vbl_style);
    m_hbl_font_all->addLayout(m_vbl_size);


    m_te_preview = new QTextEdit(this);
    m_te_preview->setText("The Quick Brown Fox Jumps Over The Lazy Dog 龙跳天门 虎卧凤阁");
    m_te_preview->setReadOnly(true);


    QStyle* style = this->style();
    m_btn_ok = new QPushButton("OK", this);
    m_btn_ok->setIcon(style->standardIcon(QStyle::SP_DialogOkButton));
    m_btn_cancel = new QPushButton("Cancel", this);
    m_btn_cancel->setIcon(style->standardIcon(QStyle::SP_DialogCancelButton));

    m_hbl_btn = new QHBoxLayout;
    m_hbl_btn->addStretch();
    m_hbl_btn->addWidget(m_btn_ok);
    m_hbl_btn->addWidget(m_btn_cancel);


    m_vbl_main = new QVBoxLayout;
    m_vbl_main->addLayout(m_hbl_font_all);
    m_vbl_main->addWidget(m_te_preview);
    m_vbl_main->addStretch();
    m_vbl_main->addLayout(m_hbl_btn);

    this->setLayout(m_vbl_main);
}