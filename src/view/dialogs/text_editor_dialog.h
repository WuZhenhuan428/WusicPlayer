#pragma once

#include <QFontComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

// no line number yet!
/*
|-------------------------------------------|
|                             (font) (size) |
|-------------------------------------------|
|1| editor...                               |
|2|                                         |
|3|                                         |
|4|                                         |
|5|                                         |
|6|                                         |
|7|                                         |
|8|                                         |
|9|                                         |
|-------------------------------------------|
|                  SaveAs   Apply   Cancel  |
|-------------------------------------------|

*/

class TextEditorDialog : public QWidget
{
    Q_OBJECT

public:
    explicit TextEditorDialog(const QString& string, QWidget* parent = nullptr);

    void init_ui();
    void init_connections();

    bool set_content(const QString& str);

protected:
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void sgn_text(const QString& text);

private:
    QLabel* lb_font_              = nullptr;
    QFontComboBox* cb_font_       = nullptr;
    QLabel* lb_font_size_         = nullptr;
    QSpinBox* sb_font_size_       = nullptr;
    QHBoxLayout* hbl_settings_    = nullptr;

    QTextEdit* text_editor_       = nullptr;

    QPushButton* btn_import_file_ = nullptr;
    QPushButton* btn_save_as_     = nullptr;
    QPushButton* btn_apply_       = nullptr;
    QPushButton* btn_cancel_      = nullptr;
    QHBoxLayout* hbl_btn_         = nullptr;

    QVBoxLayout* vbl_main_        = nullptr;
};
