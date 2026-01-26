#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QString>

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