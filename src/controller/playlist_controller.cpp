#include "controller/playlist_controller.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringList>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("playlist_controller", {"console", "gui"});
}

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

    connect(m_manager, &PlaylistManager::sgn_playlist_changed, this,
            &PlaylistController::sgn_playlist_changed);
    connect(m_manager, &PlaylistManager::sgn_request_play, this,
            &PlaylistController::sgn_request_play);
    connect(m_manager, &PlaylistManager::sgn_cache_load_finished, this,
            &PlaylistController::sgn_cache_load_finished);
    connect(m_manager->m_context, &PlaylistContext::sgn_current_play_mode_changed, this,
            &PlaylistController::sgn_play_mode_changed);
}

PlaylistController::~PlaylistController() {}

namespace
{
PlaylistId checkId(const PlaylistManager* manager, const PlaylistId& pid)
{
    PlaylistId curr_pid;
    auto curr_playlist = manager->m_repo->find_playlist_by_id(pid);
    if (nullptr != curr_playlist) {
        curr_pid = pid;
    } else {
        curr_pid = manager->m_context->get_playlist_id(); // default use playing playlist
    }
    return curr_pid;
}
}; // namespace

void PlaylistController::import_files(const PlaylistId& pid)
{
    if (!m_manager)
        return;
    PlaylistId target_id    = checkId(m_manager, pid);

    // 单文件默认仅外部文件;策略可配置/询问
    const AddFilePolicy eff = resolve_policy(AddFilePolicy::keep_external);
    QStringList files       = QFileDialog::getOpenFileNames(
        m_dialogParent, tr("Open Audio Files"), QString(),
        tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a);;All Files (*)"));
    if (!files.isEmpty()) {
        for (const auto& file : files) {
            m_manager->add_track(target_id, file, eff);
        }
    }
}

void PlaylistController::import_dir(const PlaylistId& pid)
{
    if (!m_manager)
        return;
    PlaylistId target_id    = checkId(m_manager, pid);

    // 文件夹默认同步入库(注册库根目录);策略可配置/询问
    const AddFilePolicy eff = resolve_policy(AddFilePolicy::import_to_library);
    QString dir = QFileDialog::getExistingDirectory(m_dialogParent, tr("Open Directory"), QString(),
                                                    QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_manager->add_folder(target_id, dir, eff);
    }
}

void PlaylistController::set_add_file_policy(AddFilePolicy policy)
{
    if (m_manager) {
        m_manager->set_add_file_policy(policy);
    }
}

AddFilePolicy PlaylistController::add_file_policy() const
{
    return m_manager ? m_manager->add_file_policy() : AddFilePolicy::by_operation;
}

AddFilePolicy PlaylistController::resolve_policy(AddFilePolicy by_operation_default) const
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

void PlaylistController::create_new_playlist()
{
    if (!m_manager)
        return;
    m_manager->create_playlist();
}

void PlaylistController::load_playlist()
{
    if (!m_manager)
        return;

    QString path = QFileDialog::getOpenFileName(m_dialogParent, tr("Open Playlist File"), QString(),
                                                tr("WusicPlayer playlist (*.wcpl)"));
    if (!path.isEmpty()) {
        m_manager->load_playlist(path);

        SortRule rule;
        rule.type = SortType::album;
        m_manager->get_view_model()->set_single_grouping(rule);
    }
}

void PlaylistController::rename_playlist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    PlaylistId target_id = id.is_null() ? m_manager->get_current_playlist_id() : id;
    if (target_id.is_null())
        return;
    QString old_name = m_manager->get_playlist_by_id(target_id);

    bool ok;
    QString new_name = QInputDialog::getText(m_dialogParent, tr("Rename Playlist"), tr("New name:"),
                                             QLineEdit::Normal, old_name, &ok);
    if (ok && !new_name.isEmpty()) {
        m_manager->rename_playlist(target_id, new_name);
    }
}

void PlaylistController::rename_playlist(const PlaylistId& id, const QString& new_name)
{
    if (!m_manager || new_name.trimmed().isEmpty()) {
        return;
    }
    const PlaylistId target_id = id.is_null() ? m_manager->get_current_playlist_id() : id;
    if (target_id.is_null()) {
        return;
    }
    m_manager->rename_playlist(target_id, new_name.trimmed());
}

void PlaylistController::remove_playlist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    PlaylistId target_id = id.is_null() ? m_manager->get_current_playlist_id() : id;
    if (target_id.is_null())
        return;

    auto btn = QMessageBox::question(m_dialogParent, tr("Confirm Remove"),
                                     tr("Do you really want to remove this playlist?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        m_manager->remove_playlist(target_id);
    }
}

void PlaylistController::save_playlist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    const PlaylistId target_id = id.is_null() ? m_manager->get_current_playlist_id() : id;
    if (target_id.is_null())
        return;

    QFileDialog dialog(m_dialogParent);
    dialog.setWindowTitle(tr("Save playlist file"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(QStringList() << tr("WusicPlayer playlist (*.wcpl)"));

    if (!dialog.exec())
        return;

    QString filename = dialog.selectedFiles().first();
    if (!filename.endsWith(".wcpl", Qt::CaseInsensitive)) {
        filename += ".wcpl";
    }

    m_manager->save_playlist(target_id, filename);
    logger->info("[PLAYLIST] playlist save to {}", filename);
}

void PlaylistController::copy_playlist(const PlaylistId& id)
{
    if (!m_manager)
        return;

    const PlaylistId target_id = id.is_null() ? m_manager->get_current_playlist_id() : id;
    if (!target_id.is_null()) {
        m_manager->copy_playlist(target_id);
    }
}

void PlaylistController::remove_track(const EntryId& id)
{
    if (!m_manager || id.is_null()) {
        return;
    }

    auto btn = QMessageBox::question(m_dialogParent, tr("Confirm Remove"),
                                     tr("Do you really want to remove this track?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        m_manager->remove_track(id);
    }
}

int PlaylistController::add_library_tracks(const PlaylistId& pid, const QVector<TrackId>& track_ids)
{
    return m_manager ? m_manager->add_library_tracks(pid, track_ids) : 0;
}

bool PlaylistController::locate_filepath(const QString& filepath)
{
    if (!m_manager || filepath.isEmpty()) {
        return false;
    }
    const auto pl = current_playlist();
    if (!pl) {
        return false;
    }
    const Track* track = pl->find_track_by_filepath(filepath);
    if (!track) {
        return false;
    }
    // 仅更新 context(play_track → view 高亮/元数据同步);不播放,由调用方统一播放
    m_manager->set_current_track(track->entry_id);
    return true;
}

void PlaylistController::remove_tracks(const QVector<EntryId>& ids)
{
    // 批量移除:不逐个弹窗确认(单选仍走带确认的 remove_track)
    if (!m_manager) {
        return;
    }
    for (const EntryId& id : ids) {
        if (!id.is_null()) {
            m_manager->remove_track(id);
        }
    }
}

int PlaylistController::copy_tracks_to_playlist(const PlaylistId& src_pid,
                                                const QVector<EntryId>& entry_ids,
                                                const PlaylistId& dst_pid)
{
    return m_manager ? m_manager->copy_tracks_to_playlist(src_pid, entry_ids, dst_pid) : 0;
}

void PlaylistController::reorder_playlists(const QVector<PlaylistId>& ordered_ids)
{
    if (m_manager) {
        m_manager->reorder_playlists(ordered_ids);
    }
}

void PlaylistController::remove_missing_tracks()
{
    if (m_manager) {
        m_manager->remove_missing_tracks();
    }
}

auto PlaylistController::view_model() const
    -> decltype(std::declval<PlaylistManager*>()->get_view_model())
{
    return m_manager->get_view_model();
}

EntryId PlaylistController::next_track() const
{
    return m_manager->next_track(m_manager->m_context->get_play_mode());
}
EntryId PlaylistController::prev_track() const
{
    return m_manager->prev_track(m_manager->m_context->get_play_mode());
}
QString PlaylistController::track_file_path(const EntryId& eid) const
{
    if (eid.is_null() || !m_manager) {
        return QString();
    }
    const PlaylistId pid = m_manager->get_current_playlist_id();
    if (pid.is_null()) {
        return QString();
    }
    const auto playlist = m_manager->m_repo->find_playlist_by_id(pid);
    if (!playlist) {
        return QString();
    }
    const Track* track = playlist->find_track_by_id(eid);
    return track ? track->filepath : QString();
}
void PlaylistController::play(int queueIndex)
{
    m_manager->play(queueIndex);
}
void PlaylistController::switch_to_playlist(const PlaylistId& id)
{
    m_manager->switch_to_playlist(id);
}

void PlaylistController::set_play_mode(PlayMode mode)
{
    if (!m_manager)
        return;
    m_manager->m_context->set_play_mode(mode);
}

PlayMode PlaylistController::play_mode() const
{
    return m_manager ? m_manager->m_context->get_play_mode() : PlayMode::in_order;
}

const QVector<std::shared_ptr<Playlist>> PlaylistController::playlists() const
{
    return m_manager->get_playlists();
}
PlaylistId PlaylistController::current_playlist_id() const
{
    return m_manager->get_current_playlist_id();
}
EntryId PlaylistController::current_track_id() const
{
    return m_manager->get_current_track_id();
}
const TrackMetaData PlaylistController::current_metadata() const
{
    return m_manager->get_current_metadata();
}
const std::shared_ptr<Playlist> PlaylistController::current_playlist()
{
    // return a share_ptr will cause the ref count increase
    return this->find_playlist_by_id(this->current_playlist_id());
}

std::shared_ptr<Playlist> PlaylistController::find_playlist_by_id(PlaylistId pid)
{
    if (pid.is_null()) {
        return nullptr;
    }
    return m_manager->m_repo->find_playlist_by_id(pid);
}

void PlaylistController::load_cache_after_shown()
{
    m_manager->load_cache_after_shown();
}

void PlaylistController::set_group_rules(const QVector<SortRule>& rules)
{
    m_manager->get_view_model()->set_group_rules(rules);
}

void PlaylistController::set_sort_rules(const QVector<SortRule>& rules)
{
    m_manager->get_view_model()->set_sort_rules(rules);
}

const QVector<SortRule> PlaylistController::group_rules() const
{
    return m_manager->get_view_model()->group_rules();
}

const QVector<SortRule> PlaylistController::sort_rules() const
{
    return m_manager->get_view_model()->sort_rules();
}

void PlaylistController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj = json.value(this->config_sub_key()).toObject();

    QVector<TableColumn> columns;
    const QJsonArray column_arr = obj.value("columns").toArray();
    for (const QJsonValue& v : column_arr) {
        if (v.isObject()) {
            columns.append(jsonToColumn(v.toObject()));
        }
    }
    this->view_model()->set_columns(columns);

    QVector<SortRule> group_rules;
    const QJsonArray group_array = obj.value("group_rules").toArray();
    for (const QJsonValue& v : group_array) {
        if (v.isObject()) {
            group_rules.append(jsonToRule(v.toObject()));
        }
    }
    this->set_group_rules(group_rules);

    QVector<SortRule> sort_rules;
    QJsonArray sort_array = obj.value("sort_rules").toArray();
    for (const QJsonValue& v : sort_array) {
        if (v.isObject()) {
            sort_rules.append(jsonToRule(v.toObject()));
        }
    }
    this->set_sort_rules(sort_rules);

    PlayMode mode =
        static_cast<PlayMode>(obj.value("play_mode").toInt(static_cast<int>(PlayMode::in_order)));
    this->set_play_mode(mode);

    const AddFilePolicy policy = static_cast<AddFilePolicy>(
        obj.value("add_file_policy").toInt(int(AddFilePolicy::by_operation)));
    this->set_add_file_policy(policy);

    m_last_playlist_id = PlaylistId(obj.value("last_playlist_id").toString());
    m_last_track_id    = EntryId(obj.value("last_track_id").toString());

    if (!m_last_playlist_id.is_null() && !this->find_playlist_by_id(m_last_playlist_id)) {
        logger->warn("playlist {} does not exist!", m_last_playlist_id.to_string());
    }
    // TODO: find track in current playlist if playlist available
}

QJsonObject PlaylistController::save_to_json()
{
    QJsonObject obj;
    QJsonArray column_array;
    for (const TableColumn& col : m_manager->get_view_model()->get_columns()) {
        column_array.append(columnToJson(col));
    }
    obj["columns"] = column_array;

    QJsonArray group_array;
    for (const SortRule& rule : m_manager->get_view_model()->group_rules()) {
        group_array.append(ruleToJson(rule));
    }
    obj["group_rules"] = group_array;

    QJsonArray sort_array;
    for (const SortRule& rule : m_manager->get_view_model()->sort_rules()) {
        sort_array.append(ruleToJson(rule));
    }
    obj["sort_rules"]       = sort_array;

    obj["play_mode"]        = static_cast<int>(this->play_mode());
    obj["add_file_policy"]  = static_cast<int>(this->add_file_policy());

    m_last_playlist_id      = this->current_playlist_id();
    m_last_track_id         = this->current_track_id();

    obj["last_playlist_id"] = m_last_playlist_id.to_string_without_brace();
    obj["last_track_id"]    = m_last_track_id.to_string_without_brace();

    return obj;
}

QString PlaylistController::config_sub_key() const
{
    return "playlist";
}

PlaylistId PlaylistController::last_playlist_id() const
{
    return m_last_playlist_id;
}

EntryId PlaylistController::last_track_id() const
{
    return m_last_track_id;
}
