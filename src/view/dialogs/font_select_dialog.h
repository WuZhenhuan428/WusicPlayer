#pragma once

#include <QDialog>

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>

#include <QStringList>
#include <QFont>
#include <QFontDatabase>

class FontSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FontSelectDialog(QFont default_font = QFontDatabase::systemFont(QFontDatabase::SystemFont::GeneralFont), QWidget* parent = nullptr);
    ~FontSelectDialog() = default;

    QFont selectFont();

private:    // data structure & functions
    void init_UI();

    QStringList m_fonts;
    QString m_family;
    QString m_style;
    int m_size;
    void init_fonts();
    void init_connections();
    void update_styles(const QString& family);
    void update_sizes(const QString& family);
    void update_preview();

private:    // widgets
    QLabel* m_lb_font;
    QListWidget* m_list_font;
    QVBoxLayout* m_vbl_font;

    QLabel* m_lb_style;
    QListWidget* m_list_style;
    QVBoxLayout* m_vbl_style;

    QLabel* m_lb_size;
    QSpinBox* m_spin_box_size;
    QListWidget* m_list_size;
    QVBoxLayout* m_vbl_size;

    QHBoxLayout* m_hbl_font_all;

    QTextEdit* m_te_preview;

    QPushButton* m_btn_ok;
    QPushButton* m_btn_cancel;
    QHBoxLayout* m_hbl_btn;

    QVBoxLayout* m_vbl_main;
};