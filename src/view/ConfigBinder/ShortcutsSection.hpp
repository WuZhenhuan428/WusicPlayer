#pragma once

#include "IConfigSection.hpp"
#include "model/ShortcutsViewModel/shortcuts_types.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

class ShortcutsSection : public IConfigSection
{
public:
    QVector<ShortcutBinding> bindings;

    QString key() const override {
        return "shortcuts";
    }

    void load(const QJsonObject& root) override {
        bindings.clear();

        const QJsonObject obj = root.value(key()).toObject();
        const QJsonArray arr = obj.value("bindings").toArray();

        for (const QJsonValue& value : arr) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject item = value.toObject();
            ShortcutActionId actionId = ShortcutActionId::play_pause;
            if (!shortcutActionIdFromString(item.value("action_id").toString(), actionId)) {
                continue;
            }

            ShortcutBinding binding;
            binding.action_id = actionId;
            binding.current_key = QKeySequence::fromString(
                item.value("key").toString(),
                QKeySequence::PortableText
            );
            binding.enabled = item.value("enabled").toBool(true);
            bindings.push_back(binding);
        }
    }

    QJsonObject save() const override {
        QJsonArray arr;
        for (const ShortcutBinding& binding : bindings) {
            QJsonObject item;
            item["action_id"] = shortcutActionIdToString(binding.action_id);
            item["key"] = binding.current_key.toString(QKeySequence::PortableText);
            item["enabled"] = binding.enabled;
            arr.append(item);
        }

        QJsonObject obj;
        obj["bindings"] = arr;
        return obj;
    }
};
