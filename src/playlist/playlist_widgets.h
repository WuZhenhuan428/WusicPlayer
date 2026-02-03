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

#include "playlist_definitions.h"

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
    WPlayListWidgetItem(QTreeWidget* parent, const QString& name, const QUuid& id)
        : QTreeWidgetItem(parent, QStringList() << name), m_id(id)
        {}
    
    QUuid id() const { return m_id; }
private:
    QUuid m_id;
};

class WInsertColumnDialog : public QDialog
{
public:
    explicit WInsertColumnDialog();
    ~WInsertColumnDialog();
    TableColumn getRule();
private:
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