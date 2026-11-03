#include "view/dialogs/text_editor_dialog.h"

#include <QFileDialog>
#include <QTextCharFormat>

TextEditorDialog::TextEditorDialog(const QString& string, QWidget* parent) : QWidget(parent)
{
    this->init_ui();
    this->init_connections();
    this->set_content(string);
    this->resize(600, 800);
}

void TextEditorDialog::init_ui()
{
    btn_import_file_ = new QPushButton("Import file", this);
    btn_save_as_     = new QPushButton("Save as", this);
    lb_font_         = new QLabel(tr("Font: "), this);
    cb_font_         = new QFontComboBox(this);
    lb_font_size_    = new QLabel(tr("Size: "), this);
    sb_font_size_    = new QSpinBox(this);
    sb_font_size_->setMinimum(5);
    sb_font_size_->setMaximum(50);
    hbl_settings_ = new QHBoxLayout();
    hbl_settings_->addWidget(btn_import_file_);
    hbl_settings_->addWidget(btn_save_as_);
    hbl_settings_->addStretch();
    hbl_settings_->addWidget(lb_font_);
    hbl_settings_->addWidget(cb_font_);
    hbl_settings_->addWidget(lb_font_size_);
    hbl_settings_->addWidget(sb_font_size_);

    text_editor_       = new QTextEdit(this);
    QFont default_font = text_editor_->font();
    cb_font_->setCurrentFont(default_font);
    sb_font_size_->setValue(default_font.pointSize() > 0 ? default_font.pointSize() : 12);

    btn_apply_  = new QPushButton("OK", this);
    btn_cancel_ = new QPushButton("Cancel", this);
    hbl_btn_    = new QHBoxLayout();
    hbl_btn_->addStretch();
    hbl_btn_->addWidget(btn_apply_);
    hbl_btn_->addWidget(btn_cancel_);

    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_settings_);
    vbl_main_->addWidget(text_editor_);
    vbl_main_->addLayout(hbl_btn_);

    this->setLayout(vbl_main_);
}

void TextEditorDialog::init_connections()
{
    // setup push buttons
    connect(btn_import_file_, &QPushButton::clicked, this, [this]() {
        QString file_name = QFileDialog::getOpenFileName(
            this, tr("Import lyrics file"), QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));

        if (file_name.isEmpty()) {
            return;
        }

        QFile file(file_name);
        if (file.open(QIODevice::ReadOnly)) {
            this->text_editor_->setText(file.readAll());
            file.close();
        }
    });

    connect(btn_save_as_, &QPushButton::clicked, this, [this]() {
        QString file_name = QFileDialog::getSaveFileName(
            this, tr("Save as"), QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));

        if (file_name.isEmpty()) {
            return;
        }

        QFile file(file_name);
        if (file.open(QIODevice::WriteOnly)) {
            QString content      = text_editor_->toPlainText();
            QByteArray str_bytes = content.toUtf8();
            file.write(str_bytes, str_bytes.length());
        }
    });

    connect(btn_apply_, &QPushButton::clicked, this, [this]() {
        emit sgn_text(text_editor_->toPlainText());
        this->close();
    });
    connect(btn_cancel_, &QPushButton::clicked, this, [this]() { this->close(); });

    // setup font and text editor (apply to existing content + future input)
    auto apply_editor_font = [this]() {
        QFont font = cb_font_->currentFont();
        font.setPointSize(sb_font_size_->value());

        QTextCharFormat fmt;
        fmt.setFontFamilies(QList<QString>() << font.family());
        fmt.setFontPointSize(font.pointSizeF());

        QTextCursor cursor = text_editor_->textCursor();
        cursor.beginEditBlock();
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(fmt);
        cursor.endEditBlock();

        text_editor_->mergeCurrentCharFormat(fmt);
        text_editor_->setCurrentFont(font);
    };

    connect(sb_font_size_, &QSpinBox::valueChanged, this,
            [apply_editor_font](int) { apply_editor_font(); });

    connect(cb_font_, &QFontComboBox::currentFontChanged, this,
            [apply_editor_font](const QFont&) { apply_editor_font(); });
}

bool TextEditorDialog::set_content(const QString& str)
{
    if (text_editor_) {
        text_editor_->setPlainText(str);
        return true;
    }

    return false;
}

void TextEditorDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
}
