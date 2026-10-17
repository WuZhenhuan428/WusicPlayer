#pragma once

#include "core/types.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QVBoxLayout>
#include <QVector>

/* usage:
    WSortTypeSetter dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        // process input text here
    }
*/
class WSortTypeSetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WSortTypeSetDialog(QWidget* parent = nullptr);
    ~WSortTypeSetDialog();

    /// 当前输入的 DSL 文本(多行)。
    QString getText() const
    {
        return txt_expression_->toPlainText();
    }
    /// 预填 DSL 文本(编辑既有规则)。
    void setText(const QString& text)
    {
        txt_expression_->setPlainText(text);
    }

private:
    QHBoxLayout* hbl_prompt_   = nullptr;
    QLabel* lb_prompt_         = nullptr;
    QTextEdit* txt_expression_ = nullptr;

    QHBoxLayout* hbl_btn_      = nullptr;
    QPushButton* btn_enter_    = nullptr;
    QPushButton* btn_cancel_   = nullptr;
    QPushButton* btn_help_     = nullptr;

    QVBoxLayout* vbl_main_     = nullptr;
};

class WPlayListWidgetItem : public QTreeWidgetItem
{
public:
    WPlayListWidgetItem(QTreeWidget* parent, const QString& name, const PlaylistId& pid) :
        QTreeWidgetItem(parent, QStringList() << name), pid_(pid)
    {
        // 支持"选中后单击"内联重命名(SelectedClicked 编辑触发器)
        setFlags(flags() | Qt::ItemIsEditable);
    }

    PlaylistId id() const
    {
        return pid_;
    }

private:
    PlaylistId pid_;
};

class WInsertColumnDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WInsertColumnDialog(const QString& title, QWidget* parent = nullptr);
    ~WInsertColumnDialog();
    TableColumn get_rule();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lb_index_        = nullptr;
    QLineEdit* le_index_     = nullptr;
    QHBoxLayout* hbl_index_  = nullptr;
    int max_index_           = 0;

    QLabel* lb_title_        = nullptr;
    QLineEdit* le_title_     = nullptr;
    QHBoxLayout* hbl_title_  = nullptr;

    QLabel* lb_type_         = nullptr;
    QComboBox* cb_type_      = nullptr;
    QHBoxLayout* hbl_type_   = nullptr;

    QPushButton* btn_ok_     = nullptr;
    QPushButton* btn_cancel_ = nullptr;
    QHBoxLayout* hbl_btn_    = nullptr;

    QLabel* lb_hint_         = nullptr;

    QVBoxLayout* vbl_main_   = nullptr;
};

class WColumnIndexDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WColumnIndexDialog(const QString& title, const QString& prompt,
                                QWidget* parent = nullptr);
    ~WColumnIndexDialog();
    void set_index(int index);
    void set_max_index(int index);
    int index() const;

private:
    QLabel* lb_prompt_       = nullptr;
    QLineEdit* txt_index_    = nullptr;
    QHBoxLayout* hbl_index_  = nullptr;
    int max_index_           = 0;

    QPushButton* btn_ok_     = nullptr;
    QPushButton* btn_cancel_ = nullptr;
    QHBoxLayout* hbl_btn_    = nullptr;

    QLabel* lb_hint_         = nullptr;

    QVBoxLayout* vbl_main_   = nullptr;
};
