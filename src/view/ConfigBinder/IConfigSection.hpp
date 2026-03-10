#pragma once

#include <QString>
#include <QJsonObject>
#include <QObject>

class IConfigSection : public QObject
{
    Q_OBJECT
public:
    virtual ~IConfigSection() = default;
    virtual QString key() const = 0;
    virtual void load(const QJsonObject& root) = 0;
    virtual QJsonObject save() const = 0;
};