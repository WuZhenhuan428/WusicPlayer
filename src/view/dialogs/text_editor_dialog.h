#pragma once

#include <QFontComboBox>
#include <QHBoxLayout>
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
    ~TextEditorDialog();

    void initUI();
    void initConnections();

    bool setContent(const QString& str);

signals:
    void sgnText(const QString& text);

private:
    QLabel* m_lb_font;
    QFontComboBox* m_cb_font;
    QLabel* m_lb_font_size;
    QSpinBox* m_sb_font_size;
    QHBoxLayout* m_hbl_settings;

    QTextEdit* m_text_editor;

    QPushButton* m_btn_import_file;
    QPushButton* m_btn_save_as;
    QPushButton* m_btn_apply;
    QPushButton* m_btn_cancel;
    QHBoxLayout* m_hbl_btn;

    QVBoxLayout* m_vbl_main;
};
