#pragma once

#include "IConfigSection.hpp"
#include <QByteArray>
#include <QJsonObject>
#include <QFont>

class DesktopLyricsSection : public IConfigSection
{
public:
    QByteArray geometry;
    bool is_visible = true;
    int rgb_active_r;
    int rgb_active_g;
    int rgb_active_b;
    int rgb_inactive_r;
    int rgb_inactive_g;
    int rgb_inactive_b;

    QString font_string;

    QString key() const override {
        return "desktop_lyrics";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
        is_visible = obj.value("is_visible").toBool(true);
        rgb_active_r = obj.value("rgb_active_r").toInt();
        rgb_active_g = obj.value("rgb_active_g").toInt();
        rgb_active_b = obj.value("rgb_active_b").toInt();
        rgb_inactive_r = obj.value("rgb_inactive_r").toInt();
        rgb_inactive_g = obj.value("rgb_inactive_g").toInt();
        rgb_inactive_b = obj.value("rgb_inactive_b").toInt();
        font_string = obj.value("font_string").toString();
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["geometry"] = QString::fromUtf8(geometry.toBase64());
        obj["is_visible"] = is_visible;
        obj["rgb_active_r"] = rgb_active_r;
        obj["rgb_active_g"] = rgb_active_g;
        obj["rgb_active_b"] = rgb_active_b;
        obj["rgb_inactive_r"] = rgb_inactive_r;
        obj["rgb_inactive_g"] = rgb_inactive_g;
        obj["rgb_inactive_b"] = rgb_inactive_b;
        obj["font_string"] = font_string;
        
        return obj;
    }
};
