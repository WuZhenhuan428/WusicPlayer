#pragma once

#include <QString>
#include <QJsonObject>

class IConfigSection
{
public:
    virtual ~IConfigSection() = default;
    virtual QString key() const = 0;
    virtual void load(const QJsonObject& root) = 0;
    virtual QJsonObject save() const = 0;
};