#include "PlaylistController.h"

#include <QInputDialog>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>

PlaylistController::PlaylistController(PlaylistManager* manager, QWidget* dialog_parent, QObject* parent)
    : QObject(parent), m_manager(manager), m_dialogParent(dialog_parent)
{
    if (!m_manager) return;

    connect(m_manager, &PlaylistManager::playlistChanged, this, &PlaylistController::playlistChanged);
    connect(m_manager, &PlaylistManager::requestPlay, this, &PlaylistController::requestPlay);
    connect(m_manager, &PlaylistManager::cacheLoadFinished, this, &PlaylistController::cacheLoadFinished);
}

PlaylistController::~PlaylistController() {}

void PlaylistController::importFiles() {
    if (!m_manager) return;

    QStringList files = QFileDialog::getOpenFileNames(
        m_dialogParent,
        "Open Audio Files",
        QString(),
        tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a);;All Files (*)")
    );
    if (!files.isEmpty()) {
        for (const auto& file : files) {
            m_manager->addTrack(file);
        }
    }
}

void PlaylistController::importDir() {
    if (!m_manager) return;

    QString dir = QFileDialog::getExistingDirectory(
        m_dialogParent,
        tr("Open Directory"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        m_manager->addFolder(dir);
    }
}

void PlaylistController::createNewPlaylist() {
    if (!m_manager) return;
    m_manager->createPlaylist();
}

void PlaylistController::loadPlaylist() {
    if (!m_manager) return;

    QString path = QFileDialog::getOpenFileName(
        m_dialogParent,
        tr("Open Playlist File"),
        QString(),
        tr("WusicPlayer playlist (*.wcpl)")
    );
    if (!path.isEmpty()) {
        m_manager->loadPlaylist(path);

        SortRule rule;
        rule.type = SortType::album;
        m_manager->getViewModel()->setSingleGrouping(rule);
    }
}

void PlaylistController::renamePlaylist(playlistId id) {
    if (!m_manager) return;

    playlistId target_id = id.isNull() ? m_manager->getCurrentPlaylist() : id;
    if (target_id.isNull()) return;
    QString old_name = m_manager->getPlaylistById(target_id);

    bool ok;
    QString new_name = QInputDialog::getText(
        m_dialogParent,
        tr("Rename Playlist"),
        tr("New name:"),
        QLineEdit::Normal,
        old_name,
        &ok
    );
    if (ok && !new_name.isEmpty()) {
        m_manager->renamePlaylist(target_id, new_name);
    }
}

void PlaylistController::removePlaylist(playlistId id) {
    if (!m_manager) return;

    playlistId target_id = id.isNull() ? m_manager->getCurrentPlaylist() : id;
    if (target_id.isNull()) return;

    auto btn = QMessageBox::question(
        m_dialogParent,
        tr("Confirm Remove"),
        tr("Do you really want to remove this playlist?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (btn == QMessageBox::Yes) {
        m_manager->removePlaylist(target_id);
    }
}

void PlaylistController::savePlaylist(playlistId id) {
    if (!m_manager) return;

    const playlistId target_id = id.isNull() ? m_manager->getCurrentPlaylist() : id;
    if (target_id.isNull()) return;

    QFileDialog dialog(m_dialogParent);
    dialog.setWindowTitle("Save playlist file");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(QStringList() << tr("WusicPlayer playlist (*.wcpl)"));

    if (!dialog.exec()) return;

    QString filename = dialog.selectedFiles().first();
    if (!filename.endsWith(".wcpl", Qt::CaseInsensitive)) {
        filename += ".wcpl";
    }

    m_manager->savePlaylist(target_id, filename);
    qDebug() << "[PLAYLIST] playlist save to" << filename;
}

void PlaylistController::copyPlaylist(playlistId id) {
    if (!m_manager) return;

    const playlistId target_id = id.isNull() ? m_manager->getCurrentPlaylist() : id;
    if (!target_id.isNull()) {
        m_manager->copyPlaylist(target_id);
    }
}

auto PlaylistController::viewModel() const -> decltype(std::declval<PlaylistManager*>()->getViewModel()) {
    return m_manager->getViewModel();
}

QString PlaylistController::nextTrack(PlayMode mode) const { return m_manager->nextTrack(mode); }
QString PlaylistController::prevTrack(PlayMode mode) const { return m_manager->prevTrack(mode); }
void PlaylistController::play(int queueIndex) { m_manager->play(queueIndex); }
void PlaylistController::switchToPlaylist(const playlistId& id) { m_manager->switchToPlaylist(id); }

const QVector<std::shared_ptr<Playlist>> PlaylistController::playlists() const {
    return m_manager->getPlaylists();
}
playlistId PlaylistController::currentPlaylist() const { return m_manager->getCurrentPlaylist(); }
trackId PlaylistController::currentTrackId() const { return m_manager->getCurrentTrackId(); }
TrackMetaData PlaylistController::currentMetadata() const { return m_manager->getCurrentMetadata(); }

void PlaylistController::loadCacheAfterShown() { m_manager->loadCacheAfterShown(); }
