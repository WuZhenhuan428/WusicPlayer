#include "view/dialogs/log_viewer_dialog.h"

#include "core/logger/log_sink_gui.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

LogViewerDialog::LogViewerDialog(LogSinkGui* sink, QWidget* parent) : QDialog(parent), m_sink(sink)
{
    setWindowTitle(tr("Log Viewer"));
    resize(680, 420);
    build_ui();
    set_sink(sink);
}

void LogViewerDialog::build_ui()
{
    m_cb_level_filter = new QComboBox;
    m_cb_level_filter->addItem(tr("All"), -1);
    m_cb_level_filter->addItem(tr("Debug+"), int(LogLevel::debug));
    m_cb_level_filter->addItem(tr("Info+"), int(LogLevel::info));
    m_cb_level_filter->addItem(tr("Warn+"), int(LogLevel::warn));
    m_cb_level_filter->addItem(tr("Error+"), int(LogLevel::error));

    m_btn_clear  = new QPushButton(tr("Clear"));
    m_chk_follow = new QCheckBox(tr("Follow"));
    m_chk_follow->setChecked(true);

    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->addWidget(new QLabel(tr("Level:")));
    toolbar->addWidget(m_cb_level_filter);
    toolbar->addStretch(1);
    toolbar->addWidget(m_chk_follow);
    toolbar->addWidget(m_btn_clear);

    m_text = new QPlainTextEdit;
    m_text->setReadOnly(true);
    m_text->setMaximumBlockCount(5000);
    m_text->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_text);

    connect(m_cb_level_filter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LogViewerDialog::on_level_filter_changed);
    connect(m_btn_clear, &QPushButton::clicked, this, &LogViewerDialog::on_clear_clicked);
}

void LogViewerDialog::set_sink(LogSinkGui* sink)
{
    if (m_sink) {
        disconnect(m_sink, nullptr, this, nullptr);
    }
    m_sink = sink;
    if (!m_sink) {
        return;
    }
    // 回显历史缓冲
    const QVector<LogRecord> history = m_sink->snapshot();
    for (const LogRecord& r : history) {
        append_line(static_cast<int>(r.level), r.name, r.message);
    }
    // 实时订阅(跨线程安全:QueuedConnection → 主线程)
    connect(m_sink, &LogSinkGui::sgn_record, this, &LogViewerDialog::on_record,
            Qt::QueuedConnection);
}

void LogViewerDialog::on_record(int level, QString module, QString message)
{
    if (!should_display(level)) {
        return;
    }
    append_line(level, module, message);
}

void LogViewerDialog::on_level_filter_changed()
{
    // 简化:过滤只影响后续追加,不回刷历史
}

void LogViewerDialog::on_clear_clicked()
{
    m_text->clear();
    if (m_sink) {
        m_sink->clear_buffer();
    }
}

bool LogViewerDialog::should_display(int level) const
{
    if (!m_cb_level_filter) {
        return true;
    }
    const int threshold = m_cb_level_filter->currentData().toInt();
    return threshold < 0 || level >= threshold;
}

void LogViewerDialog::append_line(int level, const QString& module, const QString& message)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const QString line =
        QStringLiteral("[%1] [%2] [%3] %4").arg(ts, level_name(LogLevel(level)), module, message);
    m_text->appendPlainText(line);
    if (m_chk_follow && m_chk_follow->isChecked()) {
        QTextCursor cursor = m_text->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_text->setTextCursor(cursor);
    }
}
