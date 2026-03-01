#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QString>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QString>

#include <QVector>
#include <utility>

#include "core/types.h"

/* usage:
    WSortTypeSetter dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        // process input text here
    }
*/
class WSortTypeSetDialog : public QDialog
{
public:
    explicit WSortTypeSetDialog(QDialog *parent = nullptr);
    ~WSortTypeSetDialog();

    QString getText() const {return txtExpression->text();}
private:
    QLabel* lbPrompt;
    QLineEdit* txtExpression;

    QPushButton* btnEnter;
    QPushButton* btnCancel;
    QPushButton* btnHelp;

    QSlider* sldMain;
    QSlider* sldBtn;
    QHBoxLayout* hbPrompt;
    QHBoxLayout* hbBtn;
    QVBoxLayout* vbMain;
};


class WPlayListWidgetItem : public QTreeWidgetItem
{
public:
    WPlayListWidgetItem(QTreeWidget* parent, const QString& name, const playlistId& pid)
        : QTreeWidgetItem(parent, QStringList() << name), m_pid(pid)
        {}
    
    playlistId id() const { return m_pid; }
private:
    playlistId m_pid;
};

class WInsertColumnDialog : public QDialog
{
public:
    explicit WInsertColumnDialog();
    ~WInsertColumnDialog();
    TableColumn getRule();
    void setIndex(int index);
    void setMaxIndex(int index);
    int index() const;
private:
    QLabel* lbIndex;
    QLineEdit* txtIndex;
    QHBoxLayout* hbIndex;
    int m_maxIndex = 0;

    QLabel* lbTitle;
    QLineEdit* txtTitle;
    QHBoxLayout* hbTitle;

    QLabel* lbType;
    QComboBox* cbType;
    QHBoxLayout* hbType;

    QPushButton* btnOK;
    QPushButton* btnCancel;
    QHBoxLayout* hbBtn;

    QVBoxLayout* vbMain;
};

class WColumnIndexDialog : public QDialog
{
public:
    explicit WColumnIndexDialog(const QString& title, const QString& prompt, QWidget* parent = nullptr);
    ~WColumnIndexDialog();
    void setIndex(int index);
    void setMaxIndex(int index);
    int index() const;

private:
    QLabel* lbPrompt;
    QLineEdit* txtIndex;
    QHBoxLayout* hbIndex;
    int m_maxIndex = 0;

    QPushButton* btnOK;
    QPushButton* btnCancel;
    QHBoxLayout* hbBtn;

    QVBoxLayout* vbMain;
};