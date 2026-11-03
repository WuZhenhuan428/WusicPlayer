#pragma once

#include <QDialog>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

class FontSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FontSelectDialog(const QFont& default_font =
                                  QFontDatabase::systemFont(QFontDatabase::SystemFont::GeneralFont),
                              QWidget* parent = nullptr);

    QFont select_font();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private: // data structure & functions
    void init_ui();

    QStringList fonts_;
    QString family_;
    QString style_;
    int size_;
    void init_fonts();
    void init_connections();
    void update_styles(const QString& family);
    void update_sizes(const QString& family);
    void update_preview();

private: // widgets
    QLabel* lb_font_           = nullptr;
    QListWidget* list_font_    = nullptr;
    QVBoxLayout* vbl_font_     = nullptr;

    QLabel* lb_style_          = nullptr;
    QListWidget* list_style_   = nullptr;
    QVBoxLayout* vbl_style_    = nullptr;

    QLabel* lb_size_           = nullptr;
    QSpinBox* sb_size_         = nullptr;
    QListWidget* list_size_    = nullptr;
    QVBoxLayout* vbl_size_     = nullptr;

    QHBoxLayout* hbl_font_all_ = nullptr;

    QTextEdit* te_preview_     = nullptr;

    QPushButton* btn_ok_       = nullptr;
    QPushButton* btn_cancel_   = nullptr;
    QHBoxLayout* hbl_btn_      = nullptr;

    QVBoxLayout* vbl_main_     = nullptr;
};
