#include "PlaylistController.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>

namespace
{
TableColumn jsonToColumn(const QJsonObject& obj)
{
    TableColumn col;
    col.headerName = obj.value("header").toString();
    col.sortType =
        static_cast<SortType>(obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted)));
    return col;
}

QJsonObject columnToJson(const TableColumn& col)
{
    QJsonObject obj;
    obj["header"]    = col.headerName;
    obj["sort_type"] = static_cast<int>(col.sortType);
    return obj;
}

SortRule jsonToRule(const QJsonObject& obj)
{
    SortRule rule{};

    rule.type =
        static_cast<SortType>(obj.value("sort_type").toInt(static_cast<int>(SortType::not_sorted)));

    rule.order =
        static_cast<Qt::SortOrder>(obj.value("order").toInt(static_cast<int>(Qt::AscendingOrder)));
    return rule;
}

QJsonObject ruleToJson(const SortRule& rule)
{
    QJsonObject obj;
    // 按你的 SortRule 字段名调整
    obj["sort_type"] = static_cast<int>(rule.type);
    obj["order"]     = static_cast<int>(rule.order);
    return obj;
}
}; // namespace

PlaylistController::PlaylistController(PlaylistManager* manager, QWidget* dialog_parent,
                                       QObject* parent) :
    QObject(parent), m_manager(manager), m_dialogParent(dialog_parent)
{
    if (!m_manager)
        return;

    connect(m_manager, &PlaylistManager::playlistChanged, this,
            &PlaylistController::playlistChanged);
    connect(m_manager, &PlaylistManager::requestPlay, this, &PlaylistController::requestPlay);
    connect(m_manager, &PlaylistManager::cacheLoadFinished, this,
            &PlaylistController::cacheLoadFinished);
    connect(m_manager->m_context, &PlaylistContext::changedCurrentPlayMode, this,
            &PlaylistController::playModeChanged);
}

PlaylistController::~PlaylistController() {}

namespace
{
PlaylistId checkId(const PlaylistManager* manager, const PlaylistId& pid)
{
    PlaylistId curr_pid;
    auto curr_playlist = manager->m_repo->findPlaylistById(pid);
    if (nullptr != curr_playlist) {
        curr_pid = pid;
    } else {
        curr_pid = manager->m_context->getPlaylistId(); // default use playing playlist
    }
    return curr_pid;
}
}; // namespace

void PlaylistController::importFiles(const PlaylistId& pid)
{
    if (!m_manager)
        return;
    PlaylistId target_id    = checkId(m_manager, pid);

    // 单文件默认仅外部文件;策略可配置/询问
    const AddFilePolicy eff = resolvePolicy(AddFilePolicy::keep_external);
    QStringList files       = QFileDialog::getOpenFileNames(
        m_dialogParent, tr("Open Audio Files"), QString(),
        tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a);;All Files (*)"));
    if (!files.isEmpty()) {
        for (const auto& file : files) {
            m_manager->addTrack(target_id, file, eff);
        }
    }
}

void PlaylistController::importDir(const PlaylistId& pid)
{
    if (!m_manager)
        return;
    PlaylistId target_id    = checkId(m_manager, pid);

    // 文件夹默认同步入库(注册库根目录);策略可配置/询问
    const AddFilePolicy eff = resolvePolicy(AddFilePolicy::import_to_library);
    QString dir = QFileDialog::getExistingDirectory(m_dialogParent, tr("Open Directory"), QString(),
                                                    QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_manager->addFolder(target_id, dir, eff);
    }
}

void PlaylistController::setAddFilePolicy(AddFilePolicy policy)
{
    if (m_manager) {
        m_manager->set_add_file_policy(policy);
    }
}

AddFilePolicy PlaylistController::addFilePolicy() const
{
    return m_manager ? m_manager->add_file_policy() : AddFilePolicy::by_operation;
}

AddFilePolicy PlaylistController::resolvePolicy(AddFilePolicy by_operation_default) const
{
    if (!m_manager) {
        return by_operation_default;
    }
    const AddFilePolicy cfg = m_manager->add_file_policy();
    switch (cfg) {
    case AddFilePolicy::import_to_library:
    case AddFilePolicy::keep_external:
        return cfg;
    case AddFilePolicy::always_ask: {
        const auto btn = QMessageBox::question(
            m_dialogParent, tr("Add to Library"),
            tr("File is not in the music library.\nAdd to library (library reference), or keep "
               "as external file?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
        if (btn == QMessageBox::Yes) {
            return AddFilePolicy::import_to_library;
        }
        if (btn == QMessageBox::No) {
            return AddFilePolicy::keep_external;
        }
        return by_operation_default; // Cancel:按操作类型默认
    }
    default: // by_operation
        return by_operation_default;
    }
}

void PlaylistController::createNewPlaylist()
{
    if (!m_manager)
        return;
    m_manager->createPlaylist();
}

void PlaylistController::loadPlaylist()
{
    if (!m_manager)
        return;

    QString path = QFileDialog::getOpenFileName(m_dialogParent, tr("Open Playlist File"), QString(),
                                                tr("WusicPlayer playlist (*.wcpl)"));
    if (!path.isEmpty()) {
        m_manager->loadPlaylist(path);

        SortRule rule;
        rule.type = SortType::album;
        m_manager->getViewModel()->setSingleGrouping(rule);
    }
}

void PlaylistController::renamePlaylist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    PlaylistId target_id = id.isNull() ? m_manager->getCurrentPlaylistId() : id;
    if (target_id.isNull())
        return;
    QString old_name = m_manager->getPlaylistById(target_id);

    bool ok;
    QString new_name = QInputDialog::getText(m_dialogParent, tr("Rename Playlist"), tr("New name:"),
                                             QLineEdit::Normal, old_name, &ok);
    if (ok && !new_name.isEmpty()) {
        m_manager->renamePlaylist(target_id, new_name);
    }
}

void PlaylistController::removePlaylist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    PlaylistId target_id = id.isNull() ? m_manager->getCurrentPlaylistId() : id;
    if (target_id.isNull())
        return;

    auto btn = QMessageBox::question(m_dialogParent, tr("Confirm Remove"),
                                     tr("Do you really want to remove this playlist?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        m_manager->removePlaylist(target_id);
    }
}

void PlaylistController::savePlaylist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    const PlaylistId target_id = id.isNull() ? m_manager->getCurrentPlaylistId() : id;
    if (target_id.isNull())
        return;

    QFileDialog dialog(m_dialogParent);
    dialog.setWindowTitle("Save playlist file");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(QStringList() << tr("WusicPlayer playlist (*.wcpl)"));

    if (!dialog.exec())
        return;

    QString filename = dialog.selectedFiles().first();
    if (!filename.endsWith(".wcpl", Qt::CaseInsensitive)) {
        filename += ".wcpl";
    }

    m_manager->savePlaylist(target_id, filename);
    qDebug() << "[PLAYLIST] playlist save to" << filename;
}

void PlaylistController::copyPlaylist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    const PlaylistId target_id = id.isNull() ? m_manager->getCurrentPlaylistId() : id;
    if (!target_id.isNull()) {
        m_manager->copyPlaylist(target_id);
    }
}

void PlaylistController::removeTrack(const EntryId& id)
{
    if (!m_manager || id.isNull()) {
        return;
    }

    auto btn = QMessageBox::question(m_dialogParent, tr("Confirm Remove"),
                                     tr("Do you really want to remove this track?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        m_manager->removeTrack(id);
    }
}

void PlaylistController::removeMissingTracks()
{
    if (m_manager) {
        m_manager->removeMissingTracks();
    }
}

auto PlaylistController::viewModel() const
    -> decltype(std::declval<PlaylistManager*>()->getViewModel())
{
    return m_manager->getViewModel();
}

EntryId PlaylistController::nextTrack() const
{
    return m_manager->nextTrack(m_manager->m_context->getPlayMode());
}
EntryId PlaylistController::prevTrack() const
{
    return m_manager->prevTrack(m_manager->m_context->getPlayMode());
}
QString PlaylistController::trackFilePath(const EntryId& eid) const
{
    if (eid.isNull() || !m_manager) {
        return QString();
    }
    const PlaylistId pid = m_manager->getCurrentPlaylistId();
    if (pid.isNull()) {
        return QString();
    }
    const auto playlist = m_manager->m_repo->findPlaylistById(pid);
    if (!playlist) {
        return QString();
    }
    const Track* track = playlist->findTrackByID(eid);
    return track ? track->filepath : QString();
}
void PlaylistController::play(int queueIndex)
{
    m_manager->play(queueIndex);
}
void PlaylistController::switchToPlaylist(const PlaylistId& id)
{
    m_manager->switchToPlaylist(id);
}

void PlaylistController::setPlayMode(PlayMode mode)
{
    if (!m_manager)
        return;
    m_manager->m_context->setPlayMode(mode);
}

PlayMode PlaylistController::playMode() const
{
    return m_manager ? m_manager->m_context->getPlayMode() : PlayMode::in_order;
}

const QVector<std::shared_ptr<Playlist>> PlaylistController::playlists() const
{
    return m_manager->getPlaylists();
}
PlaylistId PlaylistController::currentPlaylistId() const
{
    return m_manager->getCurrentPlaylistId();
}
EntryId PlaylistController::currentTrackId() const
{
    return m_manager->getCurrentTrackId();
}
const TrackMetaData PlaylistController::currentMetadata() const
{
    return m_manager->getCurrentMetadata();
}
const std::shared_ptr<Playlist> PlaylistController::current_playlist()
{
    // return a share_ptr will cause the ref count increase
    return this->findPlaylistById(this->currentPlaylistId());
}

std::shared_ptr<Playlist> PlaylistController::findPlaylistById(PlaylistId pid)
{
    if (pid.isNull()) {
        return nullptr;
    }
    return m_manager->m_repo->findPlaylistById(pid);
}

void PlaylistController::loadCacheAfterShown()
{
    m_manager->loadCacheAfterShown();
}

void PlaylistController::setGroupRules(const QVector<SortRule>& rules)
{
    m_manager->getViewModel()->setGroupRules(rules);
}

void PlaylistController::setSortRules(const QVector<SortRule>& rules)
{
    m_manager->getViewModel()->setSortRules(rules);
}

const QVector<SortRule> PlaylistController::groupRules() const
{
    return m_manager->getViewModel()->groupRules();
}

const QVector<SortRule> PlaylistController::sortRules() const
{
    return m_manager->getViewModel()->sortRules();
}

void PlaylistController::loadFromJson(const QJsonObject& json)
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

    PlayMode mode =
        static_cast<PlayMode>(obj.value("play_mode").toInt(static_cast<int>(PlayMode::in_order)));
    this->setPlayMode(mode);

    const AddFilePolicy policy = static_cast<AddFilePolicy>(
        obj.value("add_file_policy").toInt(int(AddFilePolicy::by_operation)));
    this->setAddFilePolicy(policy);

    m_last_playlist_id = PlaylistId(obj.value("last_playlist_id").toString());
    m_last_track_id    = EntryId(obj.value("last_track_id").toString());

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
    obj["sort_rules"]       = sort_array;

    obj["play_mode"]        = static_cast<int>(this->playMode());
    obj["add_file_policy"]  = static_cast<int>(this->addFilePolicy());

    m_last_playlist_id      = this->currentPlaylistId();
    m_last_track_id         = this->currentTrackId();

    obj["last_playlist_id"] = m_last_playlist_id.toString(QUuid::WithoutBraces);
    obj["last_track_id"]    = m_last_track_id.toString(QUuid::WithoutBraces);

    return obj;
}

QString PlaylistController::configSubKey() const
{
    return "playlist";
}

PlaylistId PlaylistController::lastPlaylistId() const
{
    return m_last_playlist_id;
}

EntryId PlaylistController::lastTrackId() const
{
    return m_last_track_id;
}
