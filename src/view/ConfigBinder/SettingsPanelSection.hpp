#pragma once
#include "IConfigSection.hpp"
#include <QByteArray>
#include <QJsonObject>

class SettingsPanelSection : public IConfigSection
{
public:
    QByteArray geometry;

    QString key() const override {
        return "settings_panel";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["geometry"] = QString::fromUtf8(geometry.toBase64());
        return obj;
    }
};