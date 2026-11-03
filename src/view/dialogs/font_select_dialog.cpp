#include "view/dialogs/font_select_dialog.h"

#include <QStyle>
#include <algorithm>

FontSelectDialog::FontSelectDialog(const QFont& default_font, QWidget* parent) : QDialog(parent)
{
    init_ui();
    init_connections();
    init_fonts();
    family_ = default_font.family();
    style_  = default_font.styleName();
    size_   = default_font.pointSize();
    this->update_styles(family_);
    this->update_sizes(family_);
    this->update_preview();
    this->setMinimumSize(800, 500);
}

QFont FontSelectDialog::select_font()
{
    QFont font = QFontDatabase::font(family_, style_, size_);
    return font;
}

void FontSelectDialog::init_fonts()
{
    fonts_ = QFontDatabase::families(QFontDatabase::Any);
    list_font_->addItems(fonts_);
    const int family_index = static_cast<int>(fonts_.indexOf(family_));
    if (family_index >= 0) {
        list_font_->setCurrentRow(family_index);
    }
    list_font_->update();
}

void FontSelectDialog::init_connections()
{
    connect(list_font_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        family_ = item->text();
        this->update_styles(family_);
        this->update_sizes(family_);
        this->update_preview();
    });

    connect(list_style_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        style_ = item->text();
        this->update_preview();
    });

    connect(list_size_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        size_ = item->text().toInt();
        sb_size_->setValue(size_);
        this->update_preview();
    });

    connect(sb_size_, &QSpinBox::valueChanged, this, [this](int value) {
        size_ = value;
        this->update_preview();
    });

    connect(btn_ok_, &QPushButton::clicked, this, &QDialog::accept);
    connect(btn_cancel_, &QPushButton::clicked, this, &QDialog::reject);
}

void FontSelectDialog::update_styles(const QString& family)
{
    QStringList styles = QFontDatabase::styles(family);
    list_style_->clear();
    list_style_->addItems(styles);
    const int style_index = static_cast<int>(styles.indexOf(style_));
    if (style_index >= 0) {
        list_style_->setCurrentRow(style_index);
    } else if (!styles.isEmpty()) {
        list_style_->setCurrentRow(0);
        style_ = styles.first();
    }
    list_style_->update();
}

void FontSelectDialog::update_sizes(const QString& family)
{
    QList<int> sizes = QFontDatabase::pointSizes(family);
    list_size_->clear();

    if (sizes.isEmpty()) {
        sb_size_->setRange(1, 999);
        sb_size_->setValue(size_);
        list_size_->update();
        return;
    }

    int min = sizes.first();
    int max = sizes.first();
    for (int s : sizes) {
        min = std::min(s, min);
        max = std::max(s, max);
        list_size_->addItem(QString::number(s));
    }
    int valid_value = std::clamp(size_, min, max);
    size_           = valid_value;
    sb_size_->setValue(valid_value);
    sb_size_->setRange(min, max);

    int size_row = -1;
    for (int i = 0; i < sizes.size(); ++i) {
        if (sizes.at(i) == valid_value) {
            size_row = i;
            break;
        }
    }
    if (size_row >= 0) {
        list_size_->setCurrentRow(size_row);
    }
    list_size_->update();
}

void FontSelectDialog::update_preview()
{
    QFont font = QFontDatabase::font(family_, style_, size_);
    te_preview_->setFont(font);
}

void FontSelectDialog::init_ui()
{
    lb_font_   = new QLabel(tr("Fonts:"), this);
    list_font_ = new QListWidget(this);
    vbl_font_  = new QVBoxLayout;

    vbl_font_->addWidget(lb_font_);
    vbl_font_->addWidget(list_font_);

    lb_style_   = new QLabel(tr("Style:"), this);
    list_style_ = new QListWidget(this);
    vbl_style_  = new QVBoxLayout;
    vbl_style_->addWidget(lb_style_);
    vbl_style_->addWidget(list_style_);

    lb_size_   = new QLabel(tr("Size:"), this);
    sb_size_   = new QSpinBox(this);
    list_size_ = new QListWidget(this);
    vbl_size_  = new QVBoxLayout;
    vbl_size_->addWidget(lb_size_);
    vbl_size_->addWidget(sb_size_);
    vbl_size_->addWidget(list_size_);

    hbl_font_all_ = new QHBoxLayout;
    hbl_font_all_->addLayout(vbl_font_);
    hbl_font_all_->addLayout(vbl_style_);
    hbl_font_all_->addLayout(vbl_size_);

    te_preview_ = new QTextEdit(this);
    te_preview_->setText("The Quick Brown Fox Jumps Over The Lazy Dog 龙跳天门 虎卧凤阁");
    te_preview_->setReadOnly(true);

    QStyle* style = this->style();
    btn_ok_       = new QPushButton("OK", this);
    btn_ok_->setIcon(style->standardIcon(QStyle::SP_DialogOkButton));
    btn_cancel_ = new QPushButton("Cancel", this);
    btn_cancel_->setIcon(style->standardIcon(QStyle::SP_DialogCancelButton));

    hbl_btn_ = new QHBoxLayout;
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_ok_);
    hbl_btn_->addWidget(btn_cancel_);

    vbl_main_ = new QVBoxLayout;
    vbl_main_->addLayout(hbl_font_all_);
    vbl_main_->addWidget(te_preview_);
    vbl_main_->addStretch();
    vbl_main_->addLayout(hbl_btn_);

    this->setLayout(vbl_main_);
}
void FontSelectDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
}
