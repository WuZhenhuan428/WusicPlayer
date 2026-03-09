#pragma once

#include "IConfigSection.hpp"
#include <QByteArray>
#include <QJsonObject>

class DesktopLyricsSection : public IConfigSection
{
public:
    QByteArray geometry;
    bool is_visible = true;

    QString key() const override {
        return "desktop_lyrics";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
        is_visible = obj.value("is_visible").toBool(true);
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["geometry"] = QString::fromUtf8(geometry.toBase64());
        obj["is_visible"] = is_visible;
        return obj;
    }
};
