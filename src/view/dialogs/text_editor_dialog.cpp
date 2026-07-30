#include "text_editor_dialog.h"

#include <QFileDialog>
#include <QTextCharFormat>

TextEditorDialog::TextEditorDialog(const QString& string, QWidget* parent) : QWidget(parent)
{
    this->initUI();
    this->initConnections();
    this->setContent(string);
}

TextEditorDialog::~TextEditorDialog() {}

void TextEditorDialog::initUI()
{
    m_btn_import_file = new QPushButton("Import file", this);
    m_btn_save_as     = new QPushButton("Save as", this);
    m_lb_font         = new QLabel("Font: ", this);
    m_cb_font         = new QFontComboBox(this);
    m_lb_font_size    = new QLabel("Size: ", this);
    m_sb_font_size    = new QSpinBox(this);
    m_sb_font_size->setMinimum(5);
    m_sb_font_size->setMaximum(50);
    m_hbl_settings = new QHBoxLayout();
    m_hbl_settings->addWidget(m_btn_import_file);
    m_hbl_settings->addWidget(m_btn_save_as);
    m_hbl_settings->addStretch();
    m_hbl_settings->addWidget(m_lb_font);
    m_hbl_settings->addWidget(m_cb_font);
    m_hbl_settings->addWidget(m_lb_font_size);
    m_hbl_settings->addWidget(m_sb_font_size);

    m_text_editor      = new QTextEdit(this);
    QFont default_font = m_text_editor->font();
    m_cb_font->setCurrentFont(default_font);
    m_sb_font_size->setValue(default_font.pointSize() > 0 ? default_font.pointSize() : 12);

    m_btn_apply  = new QPushButton("OK", this);
    m_btn_cancel = new QPushButton("Cancel", this);
    m_hbl_btn    = new QHBoxLayout();
    m_hbl_btn->addStretch();
    m_hbl_btn->addWidget(m_btn_apply);
    m_hbl_btn->addWidget(m_btn_cancel);

    m_vbl_main = new QVBoxLayout();
    m_vbl_main->addLayout(m_hbl_settings);
    m_vbl_main->addWidget(m_text_editor);
    m_vbl_main->addLayout(m_hbl_btn);

    this->setLayout(m_vbl_main);
}

void TextEditorDialog::initConnections()
{
    // setup push buttons
    connect(m_btn_import_file, &QPushButton::clicked, this, [this]() {
        QString file_name = QFileDialog::getOpenFileName(
            this, tr("Import lyrics file"), QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));

        if (file_name.isEmpty())
            return;

        QFile file(file_name);
        if (file.open(QIODevice::ReadOnly)) {
            this->m_text_editor->setText(file.readAll());
            file.close();
        }
    });

    connect(m_btn_save_as, &QPushButton::clicked, this, [this]() {
        QString file_name = QFileDialog::getSaveFileName(
            this, tr("Save as"), QString(),
            tr("LRC files (*.lrc);;Text files (*.txt);;All files (*)"));

        if (file_name.isEmpty()) {
            return;
        }

        QFile file(file_name);
        if (file.open(QIODevice::WriteOnly)) {
            QString content      = m_text_editor->toPlainText();
            QByteArray str_bytes = content.toUtf8();
            file.write(str_bytes, str_bytes.length());
        }
    });

    connect(m_btn_apply, &QPushButton::clicked, this, [this]() {
        emit sgnText(m_text_editor->toPlainText());
        this->close();
    });
    connect(m_btn_cancel, &QPushButton::clicked, this, [this]() { this->close(); });

    // setup font and text editor (apply to existing content + future input)
    auto applyEditorFont = [this]() {
        QFont font = m_cb_font->currentFont();
        font.setPointSize(m_sb_font_size->value());

        QTextCharFormat fmt;
        fmt.setFontFamilies(QList<QString>() << font.family());
        fmt.setFontPointSize(font.pointSizeF());

        QTextCursor cursor = m_text_editor->textCursor();
        cursor.beginEditBlock();
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(fmt);
        cursor.endEditBlock();

        m_text_editor->mergeCurrentCharFormat(fmt);
        m_text_editor->setCurrentFont(font);
    };

    connect(m_sb_font_size, &QSpinBox::valueChanged, this,
            [applyEditorFont](int) { applyEditorFont(); });

    connect(m_cb_font, &QFontComboBox::currentFontChanged, this,
            [applyEditorFont](const QFont&) { applyEditorFont(); });
}

bool TextEditorDialog::setContent(const QString& str)
{
    if (m_text_editor) {
        m_text_editor->setPlainText(str);
        return true;
    }

    return false;
}
