#pragma once

#include "core/types.h"
#include "lrc_parser.h"

#include <QAbstractListModel>
#include <QPainter>
#include <QString>
#include <QVariant>
#include <QVector>

class WLyricsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum UserDefineRole
    {
        CurrentLine = Qt::UserRole + 1,
        CurrentTime
    };

    explicit WLyricsModel(QObject* parent = nullptr);
    ~WLyricsModel();

    void set_default_info(const TrackMetaData& meta);
    bool set_local_lrc(const QString& filepath);
    bool set_lrc_file_path(const QString& lrc_path);
    bool set_raw_lyrics(const QString& raw_data);
    int get_row_by_position(qint64 pos_ms);
    QString raw_lyrics_text() const;
    void set_current_position(qint64 pos_ms);
    int current_row() const;
    QString prev_line_text() const;
    QString current_line_text() const;
    QString next_line_text() const;

    // QAbstractListModel interface
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

signals:
    void currentLineChanged(const QString& curr_text, const QString& next_text);
    void sgnUseTimelineFollow(bool enable);

private:
    LrcParser m_parser;
    LrcParser::LrcFile m_lrc_file;
    QString m_raw_lyrics_text;
    int m_current_row = -1;
};
