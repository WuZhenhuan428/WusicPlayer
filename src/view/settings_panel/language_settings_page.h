#pragma once

#include <QListWidgetItem>
#include <QWidget>

class QComboBox;
class QLabel;
class LanguageSettings;

/// 界面语言设置页(English / 简体中文); 切换后重启生效。
class LanguageSettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit LanguageSettingsPage(LanguageSettings* settings, QWidget* parent = nullptr);

    QListWidgetItem* get_title_item();

private:
    LanguageSettings* m_settings = nullptr;
    QComboBox* m_cb_language     = nullptr;
    QLabel* m_lb_hint            = nullptr;
    QListWidgetItem* m_title     = nullptr;
};
