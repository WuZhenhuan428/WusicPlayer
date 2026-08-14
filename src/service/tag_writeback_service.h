#pragma once

#include "core/types.h"
#include <QObject>
#include <QPointer>

class AppContext;
class PlaylistController;
class PlaybackController;
class PlaylistManager;
class TagEditWidget;
class MainWindow;

class TagWritebackService : public QObject
{
    Q_OBJECT

public:
    explicit TagWritebackService(AppContext& ctx, QObject* parent);
    ~TagWritebackService();

    void request_track_property(EntryId tid, QString filepath, TrackMetaData meta);

private:
    AppContext& ctx_;
    PlaylistController* playlist_ctl_;
    PlaybackController* playback_ctl_;
    PlaylistManager* playlist_manager_;
    MainWindow* main_window_;
    QPointer<TagEditWidget> tag_edit_widget_;

    bool m_bound = false;
};
