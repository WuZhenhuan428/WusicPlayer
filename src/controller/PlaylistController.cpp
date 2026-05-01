#include "PlaylistController.h"

#include <QInputDialog>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>

#include <QJsonObject>
#include <QJsonArray>

namespace
{
TableColumn jsonToColumn(const QJsonObject& obj) {
    TableColumn col;
    col.headerName = obj.value("header").toString();
    col.sortType = static_cast<SortType>(
        obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted))
    );
    return col;
}

QJsonObject columnToJson(const TableColumn& col) {
    QJsonObject obj;
    obj["header"] = col.headerName;
    obj["sort_type"] = static_cast<int>(col.sortType);
    return obj;
}

SortRule jsonToRule(const QJsonObject& obj) {
    SortRule rule{};

    rule.type = static_cast<SortType>(
        obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted)));
    
    rule.order = static_cast<Qt::SortOrder>(
        obj.value("order").toInt(static_cast<int>(Qt::AscendingOrder)));
    return rule;
}

QJsonObject ruleToJson(const SortRule& rule) {
    QJsonObject obj;
    // 按你的 SortRule 字段名调整
    obj["sort_type"] = static_cast<int>(rule.type);
    obj["order"] = static_cast<int>(rule.order);
    return obj;
}
};

PlaylistController::PlaylistController(PlaylistManager* manager, QWidget* dialog_parent, QObject* parent)
    : QObject(parent), m_manager(manager), m_dialogParent(dialog_parent)
{
    if (!m_manager) return;

    connect(m_manager, &PlaylistManager::playlistChanged, this, &PlaylistController::playlistChanged);
    connect(m_manager, &PlaylistManager::requestPlay, this, &PlaylistController::requestPlay);
    connect(m_manager, &PlaylistManager::cacheLoadFinished, this, &PlaylistController::cacheLoadFinished);
}

PlaylistController::~PlaylistController() {}

namespace
{
playlistId checkId(const PlaylistManager* manager, const playlistId& pid) {
    playlistId curr_pid;
    auto curr_playlist = manager->m_repo->findPlaylistById(pid);
    if (nullptr != curr_playlist) {
        curr_pid = pid;
    } else {
        curr_pid = manager->m_context->getPlaylistId();  // default use playing playlist
    }
    return curr_pid;
}
};

void PlaylistController::importFiles(const playlistId& pid) {
    if (!m_manager) return;
    playlistId target_id = checkId(m_manager, pid);

    QStringList files = QFileDialog::getOpenFileNames(
        m_dialogParent,
        tr("Open Audio Files"),
        QString(),
        tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a);;All Files (*)")
    );
    if (!files.isEmpty()) {
        for (const auto& file : files) {
            m_manager->addTrack(target_id, file);
        }
    }
}

void PlaylistController::importDir(const playlistId& pid) {
    if (!m_manager) return;
    playlistId target_id = checkId(m_manager, pid);

    QString dir = QFileDialog::getExistingDirectory(
        m_dialogParent,
        tr("Open Directory"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        m_manager->addFolder(target_id, dir);
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

void PlaylistController::renamePlaylist(const playlistId& id) {
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

void PlaylistController::removePlaylist(const playlistId& id) {
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

void PlaylistController::savePlaylist(const playlistId& id) {
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

void PlaylistController::copyPlaylist(const playlistId& id) {
    if (!m_manager) return;

    const playlistId target_id = id.isNull() ? m_manager->getCurrentPlaylist() : id;
    if (!target_id.isNull()) {
        m_manager->copyPlaylist(target_id);
    }
}

void PlaylistController::removeTrack(const trackId& id) {
    if (!m_manager || id.isNull()) {
        return;
    }

    auto btn = QMessageBox::question(
        m_dialogParent,
        tr("Confirm Remove"),
        tr("Do you really want to remove this track?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (btn == QMessageBox::Yes) {
        m_manager->removeTrack(id);
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

std::shared_ptr<Playlist> PlaylistController::findPlaylistById(playlistId pid) {
    if (pid.isNull()) {
        return nullptr;
    }
    return m_manager->m_repo->findPlaylistById(pid);
}

void PlaylistController::loadCacheAfterShown() { m_manager->loadCacheAfterShown(); }


void PlaylistController::setGroupRules(const QVector<SortRule>& rules) {
    m_manager->getViewModel()->setGroupRules(rules);
}

void PlaylistController::setSortRules(const QVector<SortRule>& rules) {
    m_manager->getViewModel()->setSortRules(rules);
}

const QVector<SortRule> PlaylistController::groupRules() const {
    return m_manager->getViewModel()->groupRules();
}

const QVector<SortRule> PlaylistController::sortRules() const {
    return m_manager->getViewModel()->sortRules();
}

void PlaylistController::loadFromJson(const QJsonObject &json)
{
    QJsonObject obj = json.value(this->configSubKey()).toObject();
    
    QVector<TableColumn> columns;
    const QJsonArray column_arr = obj.value("columns").toArray();
    for (const QJsonValue& v : column_arr) {
        if (v.isObject()) {
            columns.append(jsonToColumn(v.toObject()));
        }
    }
    this->viewModel()->setColumns(columns);

    QVector<SortRule> group_rules;
    const QJsonArray group_array = obj.value("group_rules").toArray();
    for (const QJsonValue& v : group_array) {
        if (v.isObject()) {
            group_rules.append(jsonToRule(v.toObject()));
        }
    }
    this->setGroupRules(group_rules);

    QVector<SortRule> sort_rules;
    QJsonArray sort_array = obj.value("sort_rules").toArray();
    for (const QJsonValue& v : sort_array) {
        if (v.isObject()) {
            sort_rules.append(jsonToRule(v.toObject()));
        }
    }
    this->setSortRules(sort_rules);

    m_last_playlist_id = playlistId(obj.value("last_playlist_id").toString());
    m_last_track_id = trackId(obj.value("last_track_id").toString());

    if (!m_last_playlist_id.isNull() && !this->findPlaylistById(m_last_playlist_id)) {
        qWarning() << "playlist" << m_last_playlist_id.toString() << "does not exist!";
    }
    // TODO: find track in current playlist if playlist available
}

QJsonObject PlaylistController::saveToJson()
{
    QJsonObject obj;
    QJsonArray column_array;
    for (const TableColumn& col : m_manager->getViewModel()->getColumns()) {
        column_array.append(columnToJson(col));
    }
    obj["columns"] = column_array;

    QJsonArray group_array;
    for (const SortRule& rule : m_manager->getViewModel()->groupRules()) {
        group_array.append(ruleToJson(rule));
    }
    obj["group_rules"] = group_array;

    QJsonArray sort_array;
    for (const SortRule& rule : m_manager->getViewModel()->sortRules()) {
        sort_array.append(ruleToJson(rule));
    }
    obj["sort_rules"] = sort_array;

    m_last_playlist_id = this->currentPlaylist();
    m_last_track_id = this->currentTrackId();

    obj["last_playlist_id"] = m_last_playlist_id.toString(QUuid::WithoutBraces);
    obj["last_track_id"] = m_last_track_id.toString(QUuid::WithoutBraces);

    return obj;
}

QString PlaylistController::configSubKey() const
{
    return "playlist";
}

playlistId PlaylistController::lastPlaylistId() const
{
    return m_last_playlist_id;
}

trackId PlaylistController::lastTrackId() const
{
    return m_last_track_id;
}