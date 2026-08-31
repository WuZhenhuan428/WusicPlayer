#include "view/playlist/playlist_widgets.h"

#include "view/dialogs/font_select_dialog.h"

#include <QLineEdit>

#include <limits>

namespace
{
/// DSL 语法参考摘要(Help 按钮弹窗内容)。
QString dsl_help_text()
{
    return QStringLiteral("WusicPlayer 排序/分类 DSL 语法参考\n"
                          "==================================\n"
                          "三小节(可任意组合, 各至多一次):\n"
                          "  sort   { ... }   多键排序\n"
                          "  group  { ... }   多级分组(按属性值, 与 bucket 互斥)\n"
                          "  bucket { ... }   条件分类(if/elif/else)\n"
                          "\n"
                          "属性(大小写无关, 直接引用):\n"
                          "  title artist album album_artist genre composer comment lyrics\n"
                          "  encoder date filename filepath directory extension\n"
                          "  year track disc disc_total duration bitrate start_at index missing\n"
                          "\n"
                          "sort/group 子句(行或分号分隔):\n"
                          "  artist asc nulls last\n"
                          "  year desc\n"
                          "\n"
                          "bucket 分支:\n"
                          "  if year >= 2010 and genre == \"摇滚\" then \"现代\"\n"
                          "  elif duration < 180 then \"短\"\n"
                          "  else \"其他\"\n"
                          "\n"
                          "表达式运算符: == != < <= > >=  and or not  + - * / %  ? :\n"
                          "函数: contains(s, sub) starts_with(s, pre) ends_with(s, suf)\n"
                          "      matches(s, regex) in(v, a, b, ...) len(s) upper(s) lower(s)\n"
                          "\n"
                          "示例:\n"
                          "  sort { artist asc nulls last; year desc; title asc }\n"
                          "  bucket { if year < 1990 then \"90前\"\n"
                          "           elif year < 2010 then \"90-00s\" else \"10后\" }\n"
                          "  group { genre asc; album }");
}
} // namespace

// ==== WSortTypeSetDialog ==== //
WSortTypeSetDialog::WSortTypeSetDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Sort / Group Rules (DSL)"));

    lb_prompt_      = new QLabel(tr("Input sorting / grouping expression (DSL):"));
    txt_expression_ = new QTextEdit(this);
    txt_expression_->setMinimumHeight(140);
    txt_expression_->setLineWrapMode(QTextEdit::WidgetWidth);

    btn_enter_  = new QPushButton(tr("OK"));
    btn_cancel_ = new QPushButton(tr("Cancel"));
    btn_help_   = new QPushButton(tr("Syntax Reference"));
    hbl_prompt_ = new QHBoxLayout();
    hbl_btn_    = new QHBoxLayout();

    vbl_main_   = new QVBoxLayout();

    connect(btn_enter_, &QPushButton::clicked, this, &QDialog::accept);
    connect(btn_cancel_, &QPushButton::clicked, this, &QDialog::reject);
    connect(btn_help_, &QPushButton::clicked, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("DSL Syntax Reference"));
        auto* te = new QTextEdit(&dlg);
        te->setPlainText(dsl_help_text());
        te->setReadOnly(true);
        te->setLineWrapMode(QTextEdit::NoWrap);
        auto* closeBtn = new QPushButton(tr("Close"), &dlg);
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        auto* vbl = new QVBoxLayout(&dlg);
        vbl->addWidget(te);
        vbl->addWidget(closeBtn, 0, Qt::AlignRight);
        dlg.resize(600, 460);
        dlg.exec();
    });

    hbl_prompt_->addWidget(lb_prompt_);
    hbl_prompt_->addStretch();

    hbl_btn_->addWidget(btn_help_);
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_enter_);
    hbl_btn_->addWidget(btn_cancel_);

    vbl_main_->addSpacing(5);
    vbl_main_->addLayout(hbl_prompt_);
    vbl_main_->addWidget(txt_expression_, 1);
    vbl_main_->addSpacing(5);
    vbl_main_->addLayout(hbl_btn_);

    this->setLayout(vbl_main_);
    this->resize(560, 280);
}

WSortTypeSetDialog::~WSortTypeSetDialog() {};

// ==== WInsertColumnDialog ==== //

WInsertColumnDialog::WInsertColumnDialog()
{
    // index
    lb_index_ = new QLabel(tr("Index:"));
    le_index_ = new QLineEdit(this);
    le_index_->setText("1");
    hbl_index_ = new QHBoxLayout();
    hbl_index_->addWidget(lb_index_);
    hbl_index_->addWidget(le_index_);

    // input title
    lb_title_  = new QLabel(tr("Title:"));
    le_title_  = new QLineEdit(this);
    hbl_title_ = new QHBoxLayout();
    hbl_title_->addWidget(lb_title_);
    hbl_title_->addWidget(le_title_);

    // choose type
    lb_type_               = new QLabel(tr("Type:"));
    cb_type_               = new QComboBox();
    QVector<QString> types = {"title",    "artist",   "album",    "album artist", "genre",
                              "composer", "year",     "date",     "track",        "disc",
                              "bitrate",  "filename", "directory"};
    cb_type_->addItems(types);
    hbl_type_ = new QHBoxLayout();
    hbl_type_->addWidget(lb_type_);
    hbl_type_->addWidget(cb_type_);

    // button
    btn_ok_     = new QPushButton(tr("OK"));
    btn_cancel_ = new QPushButton(tr("Cancel"));
    hbl_btn_    = new QHBoxLayout();
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_ok_);
    hbl_btn_->addWidget(btn_cancel_);

    // main layout
    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_index_);
    vbl_main_->addLayout(hbl_title_);
    vbl_main_->addLayout(hbl_type_);
    vbl_main_->addStretch();
    vbl_main_->addLayout(hbl_btn_);
    this->setLayout(vbl_main_);

    connect(btn_ok_, &QPushButton::clicked, this, [this]() {
        bool ok         = false;
        qlonglong value = le_index_ ? le_index_->text().toLongLong(&ok) : 0;
        if (!ok || value <= 0) {
            QMessageBox::warning(this, tr("Invalid index"),
                                 tr("Index must be a positive integer (not 0)."));
            return;
        }
        if (value > std::numeric_limits<int>::max()) {
            QMessageBox::warning(this, tr("Invalid index"), tr("Index is too large."));
            return;
        }
        int intValue = static_cast<int>(value);
        if (max_index_ > 0 && intValue > max_index_) {
            QMessageBox::warning(this, tr("Invalid index"),
                                 tr("Index is out of range. Clamped to max."));
            le_index_->setText(QString::number(max_index_));
            accept();
            return;
        }
        accept();
    });
    connect(btn_cancel_, &QPushButton::clicked, this, &QDialog::reject);
}

WInsertColumnDialog::~WInsertColumnDialog() {}

void WInsertColumnDialog::set_index(int index)
{
    if (le_index_) {
        le_index_->setText(QString::number(index));
    }
}

void WInsertColumnDialog::set_max_index(int index)
{
    max_index_ = index;
}

int WInsertColumnDialog::index() const
{
    bool ok   = false;
    int value = le_index_ ? le_index_->text().toInt(&ok) : 0;
    return ok ? value : 0;
}

TableColumn WInsertColumnDialog::get_rule()
{
    TableColumn retval;
    retval.headerName = le_title_->text();
    retval.sortType   = mapStrToSorttype.value(cb_type_->currentText(), SortType::not_sorted);
    return retval;
}

// ==== WColumnIndexDialog ==== //
WColumnIndexDialog::WColumnIndexDialog(const QString& title, const QString& prompt,
                                       QWidget* parent) : QDialog(parent)
{
    setWindowTitle(title);
    lb_prompt_ = new QLabel(prompt);
    txt_index_ = new QLineEdit(this);
    txt_index_->setText("1");
    hbl_index_ = new QHBoxLayout();
    hbl_index_->addWidget(lb_prompt_);
    hbl_index_->addWidget(txt_index_);

    btn_ok_     = new QPushButton(tr("OK"));
    btn_cancel_ = new QPushButton(tr("Cancel"));
    hbl_btn_    = new QHBoxLayout();
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_ok_);
    hbl_btn_->addWidget(btn_cancel_);

    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_index_);
    vbl_main_->addStretch();
    vbl_main_->addLayout(hbl_btn_);
    setLayout(vbl_main_);

    connect(btn_ok_, &QPushButton::clicked, this, [this]() {
        bool ok         = false;
        qlonglong value = txt_index_ ? txt_index_->text().toLongLong(&ok) : 0;
        if (!ok || value <= 0) {
            QMessageBox::warning(this, tr("Invalid index"),
                                 tr("Index must be a positive integer (not 0)."));
            return;
        }
        if (value > std::numeric_limits<int>::max()) {
            QMessageBox::warning(this, tr("Invalid index"), tr("Index is too large."));
            return;
        }
        int intValue = static_cast<int>(value);
        if (max_index_ > 0 && intValue > max_index_) {
            QMessageBox::warning(this, tr("Invalid index"),
                                 tr("Index is out of range. Clamped to max."));
            txt_index_->setText(QString::number(max_index_));
            accept();
            return;
        }
        accept();
    });
    connect(btn_cancel_, &QPushButton::clicked, this, &QDialog::reject);
}

WColumnIndexDialog::~WColumnIndexDialog() {}

void WColumnIndexDialog::set_index(int index)
{
    if (txt_index_) {
        txt_index_->setText(QString::number(index));
    }
}

void WColumnIndexDialog::set_max_index(int index)
{
    max_index_ = index;
}

int WColumnIndexDialog::index() const
{
    bool ok   = false;
    int value = txt_index_ ? txt_index_->text().toInt(&ok) : 0;
    return ok ? value : 0;
}
