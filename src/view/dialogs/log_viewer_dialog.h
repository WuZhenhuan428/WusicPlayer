#pragma once

#include "core/logger/log_level.h"

#include <QDialog>
#include <QPointer>

class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class QPushButton;

class LogSinkGui;

/**
 * @brief 软件内日志查看窗口(View → Log)。
 *
 * 订阅 LogSinkGui 的 sgn_record(QueuedConnection,主线程接收);
 * 打开时回显 sink 环形缓冲中的历史记录。
 * 支持:最低级别过滤、清空、跟随滚动。
 */
class LogViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LogViewerDialog(LogSinkGui* sink, QWidget* parent = nullptr);

    // 把 sink 绑定到本窗口(由外部在创建时注入)
    void set_sink(LogSinkGui* sink);

private slots:
    void on_record(int level, QString module, QString message);
    void on_level_filter_changed();
    void on_clear_clicked();

private:
    void build_ui();
    void append_line(int level, const QString& module, const QString& message);
    bool should_display(int level) const;

    LogSinkGui* m_sink           = nullptr; // 非拥有

    QComboBox* m_cb_level_filter = nullptr;
    QPushButton* m_btn_clear     = nullptr;
    QCheckBox* m_chk_follow      = nullptr;
    QPlainTextEdit* m_text       = nullptr;
};
