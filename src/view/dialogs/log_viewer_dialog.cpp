#include "view/dialogs/log_viewer_dialog.h"

#include "core/logger/log_sink_gui.h"
#include "core/utils/font.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>
#include <utility>

LogViewerDialog::LogViewerDialog(LogSinkGui* sink, QWidget* parent) : QDialog(parent), sink_(sink)
{
    setWindowTitle(tr("Log Viewer"));
    resize(680, 420);
    build_ui();
    set_sink(sink);
}

void LogViewerDialog::build_ui()
{
    cb_level_filter_ = new QComboBox;
    cb_level_filter_->addItem(tr("All"), -1);
    cb_level_filter_->addItem(tr("Debug+"), std::to_underlying(LogLevel::Debug));
    cb_level_filter_->addItem(tr("Info+"), std::to_underlying(LogLevel::Info));
    cb_level_filter_->addItem(tr("Warn+"), std::to_underlying(LogLevel::Warn));
    cb_level_filter_->addItem(tr("Error+"), std::to_underlying(LogLevel::Error));

    btn_clear_  = new QPushButton(tr("Clear"));
    chk_follow_ = new QCheckBox(tr("Follow"));
    chk_follow_->setChecked(true);

    sb_font_size_ = new QSpinBox(this);
    sb_font_size_->setMinimum(5);
    sb_font_size_->setMaximum(50);

    hbl_toolbar_ = new QHBoxLayout;
    hbl_toolbar_->setContentsMargins(0, 0, 0, 0);
    hbl_toolbar_->addWidget(new QLabel(tr("Level:"), this));
    hbl_toolbar_->addWidget(cb_level_filter_);
    hbl_toolbar_->addStretch(1);
    hbl_toolbar_->addWidget(chk_follow_);
    hbl_toolbar_->addWidget(btn_clear_);
    hbl_toolbar_->addWidget(new QLabel(tr("Font: "), this));
    hbl_toolbar_->addWidget(sb_font_size_);

    text_ = new QPlainTextEdit;
    text_->setReadOnly(true);
    text_->setMaximumBlockCount(5000);
    text_->setLineWrapMode(QPlainTextEdit::NoWrap);
    auto mono_font = utils::font::get_system_mono_font();
    text_->setFont(mono_font);

    vbl_main_ = new QVBoxLayout;
    vbl_main_->addLayout(hbl_toolbar_);
    vbl_main_->addWidget(text_);

    this->setLayout(vbl_main_);

    sb_font_size_->setValue(mono_font.pointSize() > 0 ? mono_font.pointSize() : 12);

    connect(cb_level_filter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LogViewerDialog::on_level_filter_changed);
    connect(btn_clear_, &QPushButton::clicked, this, &LogViewerDialog::on_clear_clicked);
    connect(sb_font_size_, &QSpinBox::valueChanged, this, [this](int new_value) {
        auto font = utils::font::get_system_mono_font();
        font.setPointSize(new_value);
        text_->setFont(font);
    });
}

void LogViewerDialog::set_sink(LogSinkGui* sink)
{
    if (sink_) {
        disconnect(sink_, nullptr, this, nullptr);
    }
    sink_ = sink;
    if (!sink_) {
        return;
    }
    // 回显历史缓冲
    const QVector<LogRecord> history = sink_->snapshot();
    for (const LogRecord& r : history) {
        append_line(static_cast<int>(r.level), r.name, r.message);
    }
    // 实时订阅(跨线程安全:QueuedConnection → 主线程)
    connect(sink_, &LogSinkGui::sgn_record, this, &LogViewerDialog::on_record,
            Qt::QueuedConnection);
}

void LogViewerDialog::on_record(int level, const QString& module, const QString& message)
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
    text_->clear();
    if (sink_) {
        sink_->clear_buffer();
    }
}

bool LogViewerDialog::should_display(int level) const
{
    if (!cb_level_filter_) {
        return true;
    }
    const int threshold = cb_level_filter_->currentData().toInt();
    return threshold < 0 || level >= threshold;
}

void LogViewerDialog::append_line(int level, const QString& module, const QString& message)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const QString line =
        QStringLiteral("[%1] [%2] [%3] %4").arg(ts, level_name(LogLevel(level)), module, message);
    text_->appendPlainText(line);
    if (chk_follow_ && chk_follow_->isChecked()) {
        QTextCursor cursor = text_->textCursor();
        cursor.movePosition(QTextCursor::End);
        text_->setTextCursor(cursor);
    }
}

void LogViewerDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
}
