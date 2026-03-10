#pragma once
#include "IConfigSection.hpp"
#include <QByteArray>
#include <QJsonObject>

class WindowConfigSection : public IConfigSection
{
public:
    QByteArray geometry;
    QByteArray state;

    QString key() const override {
        return "window";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
        state = QByteArray::fromBase64(obj.value("state").toString().toUtf8());
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["geometry"] = QString::fromUtf8(geometry.toBase64());
        obj["state"] = QString::fromUtf8(state.toBase64());
        return obj;
    }
};
