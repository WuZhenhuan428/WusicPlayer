#pragma once

#include <QObject>

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"

class PlaylistManager : public QObject
{
    Q_OBJECT

    PlaylistContext* m_context = new PlaylistContext;
    PlaylistRepo* m_repo = new PlaylistRepo;
    PlaylistViewModel* m_view = new PlaylistViewModel(m_repo);

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();

public:

public slots:
    // receive signals from UI
    void createPlaylist();
    void removePlaylist();
    void copyPlaylist();
    void loadPlaylist();
    void renamePlaylist();
    void savePlaylist();

    void addTrack(const QString& filepath);


signals:

private:

};