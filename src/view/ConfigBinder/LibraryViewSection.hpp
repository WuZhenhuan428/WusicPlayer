#pragma once

#include "IConfigSection.hpp"
#include "core/types.h"
#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class LibraryViewSection : public IConfigSection
{
public:
    QByteArray song_tree_view_state = QByteArray();
    QByteArray splitter_state = QByteArray();
    Qt::Orientation splitter_orientation = Qt::Orientation::Horizontal;
    QVector<TableColumn> columns;
    QVector<SortRule> group_rules;
    QVector<SortRule> sort_rules;

    QString key() const override {
        return "library_window";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();

        song_tree_view_state = QByteArray::fromBase64(obj.value("song_tree_view_state").toString().toUtf8());
        splitter_state = QByteArray::fromBase64(obj.value("splitter_state").toString().toUtf8());
        splitter_orientation = static_cast<Qt::Orientation>(obj.value("splitter_orientation").toInt());

        columns.clear();
        const QJsonArray column_arr = obj.value("columns").toArray();
        for (const QJsonValue& v : column_arr) {
            if (v.isObject()) {
                columns.append(jsonToColumn(v.toObject()));
            }
        }

        group_rules.clear();
        const QJsonArray group_array = obj.value("group_rules").toArray();
        for (const QJsonValue& v : group_array) {
            if (v.isObject()) {
                group_rules.append(jsonToRule(v.toObject()));
            }
        }

        sort_rules.clear();
        QJsonArray sort_array = obj.value("sort_rules").toArray();
        for (const QJsonValue& v : sort_array) {
            if (v.isObject()) {
                sort_rules.append(jsonToRule(v.toObject()));
            }
        }
    }

    QJsonObject save() const override {
        QJsonObject obj;

        obj["song_tree_view_state"] = QString::fromUtf8(song_tree_view_state.toBase64());
        obj["splitter_state"] = QString::fromUtf8(splitter_state.toBase64());
        obj["splitter_orientation"] = static_cast<int>(splitter_orientation);

        QJsonArray column_array;
        for (const TableColumn& col : columns) {
            column_array.append(columnToJson(col));
        }
        obj["columns"] = column_array;

        QJsonArray group_array;
        for (const SortRule& rule : group_rules) {
            group_array.append(ruleToJson(rule));
        }
        obj["group_rules"] = group_array;

        QJsonArray sort_array;
        for (const SortRule& rule : sort_rules) {
            sort_array.append(ruleToJson(rule));
        }
        obj["sort_rules"] = sort_array;

        return obj;
    }

private:
    TableColumn jsonToColumn(const QJsonObject& obj) {
        TableColumn col;
        col.headerName = obj.value("header").toString();
        col.sortType = static_cast<SortType>(
            obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted))
        );
        return col;
    }

    QJsonObject columnToJson(const TableColumn& col) const {
        QJsonObject obj;
        obj["header"] = col.headerName;
        obj["sort_type"] = static_cast<int>(col.sortType);
        return obj;
    }

    SortRule jsonToRule(const QJsonObject& obj) const {
        SortRule rule{};

        rule.type = static_cast<SortType>(
            obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted)));
        
        rule.order = static_cast<Qt::SortOrder>(
            obj.value("order").toInt(static_cast<int>(Qt::AscendingOrder)));
        return rule;
    }

    QJsonObject ruleToJson(const SortRule& rule) const {
        QJsonObject obj;
        // 按你的 SortRule 字段名调整
        obj["sort_type"] = static_cast<int>(rule.type);
        obj["order"] = static_cast<int>(rule.order);
        return obj;
    }
    
};