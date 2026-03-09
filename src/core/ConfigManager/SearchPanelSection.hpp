#pragma once

#include "IConfigSection.hpp"
#include <QByteArray>
#include <QJsonObject>
#include <QString>


class SearchPanelSection : public IConfigSection
{
public:
    QByteArray geometry;
    QByteArray header_state;
    bool is_visible;

    QString key() const override {
        return "search_panel";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
        header_state = QByteArray::fromBase64(obj.value("header_state").toString().toUtf8());
        is_visible = obj.value("is_visible").toBool();
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["geometry"] = QString::fromUtf8(geometry.toBase64());
        obj["header_state"] = QString::fromUtf8(header_state.toBase64());
        obj["is_visible"] = is_visible;
        return obj;
    }
};