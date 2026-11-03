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
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playlist_controller", {"console", "gui"});
} // namespace

namespace
{
TableColumn json_to_column(const QJsonObject& obj)
{
    TableColumn col;
    col.header_name = obj.value("header").toString();
    col.sort_type =
        static_cast<SortType>(obj.value("sort_type").toInt(static_cast<int>(SortType::NotSorted)));
    return col;
}

QJsonObject column_to_json(const TableColumn& col)
{
    QJsonObject obj;
    obj["header"]    = col.header_name;
    obj["sort_type"] = static_cast<int>(col.sort_type);
    return obj;
}

SortRule json_to_rule(const QJsonObject& obj)
{
    SortRule rule{};

    rule.type =
        static_cast<SortType>(obj.value("sort_type").toInt(static_cast<int>(SortType::NotSorted)));

    rule.order =
        static_cast<Qt::SortOrder>(obj.value("order").toInt(static_cast<int>(Qt::AscendingOrder)));
    return rule;
}

QJsonObject rule_to_json(const SortRule& rule)
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
    QObject(parent), manager_(manager), dialog_parent_(dialog_parent)
{
    if (!manager_) {
        return;
    }

    connect(manager_, &PlaylistManager::sgn_playlist_changed, this,
            &PlaylistController::sgn_playlist_changed);
    connect(manager_, &PlaylistManager::sgn_request_play, this,
            &PlaylistController::sgn_request_play);
    connect(manager_, &PlaylistManager::sgn_cache_load_finished, this,
            &PlaylistController::sgn_cache_load_finished);
    connect(manager_->context_, &PlaylistContext::sgn_current_play_mode_changed, this,
            &PlaylistController::sgn_play_mode_changed);
}

namespace
{
PlaylistId check_id(const PlaylistManager* manager, const PlaylistId& pid)
{
    PlaylistId curr_pid;
    auto curr_playlist = manager->repo_->find_playlist_by_id(pid);
    if (nullptr != curr_playlist) {
        curr_pid = pid;
    } else {
        curr_pid = manager->context_->get_playlist_id(); // default use playing playlist
    }
    return curr_pid;
}
}; // namespace

void PlaylistController::import_files(const PlaylistId& pid)
{
    if (!manager_) {
        return;
    }

    PlaylistId target_id    = check_id(manager_, pid);
    // 单文件默认仅外部文件;策略可配置/询问
    const AddFilePolicy eff = resolve_policy(AddFilePolicy::KeepExternal);
    QStringList files       = QFileDialog::getOpenFileNames(
        dialog_parent_, tr("Open Audio Files"), QString(),
        tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a);;All Files (*)"));
    if (!files.isEmpty()) {
        for (const auto& file : files) {
            manager_->add_track(target_id, file, eff);
        }
    }
}

void PlaylistController::import_dir(const PlaylistId& pid)
{
    if (!manager_) {
        return;
    }

    PlaylistId target_id    = check_id(manager_, pid);
    // 文件夹默认同步入库(注册库根目录);策略可配置/询问
    const AddFilePolicy eff = resolve_policy(AddFilePolicy::ImportToLibrary);
    QString dir = QFileDialog::getExistingDirectory(dialog_parent_, tr("Open Directory"), QString(),
                                                    QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        manager_->add_folder(target_id, dir, eff);
    }
}

void PlaylistController::set_add_file_policy(AddFilePolicy policy)
{
    if (manager_) {
        manager_->set_add_file_policy(policy);
    }
}

AddFilePolicy PlaylistController::add_file_policy() const
{
    return manager_ ? manager_->add_file_policy() : AddFilePolicy::ByOperation;
}

AddFilePolicy PlaylistController::resolve_policy(AddFilePolicy by_operation_default) const
{
    if (!manager_) {
        return by_operation_default;
    }
    const AddFilePolicy cfg = manager_->add_file_policy();
    switch (cfg) {
    case AddFilePolicy::ImportToLibrary:
    case AddFilePolicy::KeepExternal:
        return cfg;
    case AddFilePolicy::AlwaysAsk: {
        const auto btn = QMessageBox::question(
            dialog_parent_, tr("Add to Library"),
            tr("File is not in the music library.\nAdd to library (library reference), or keep "
               "as external file?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
        if (btn == QMessageBox::Yes) {
            return AddFilePolicy::ImportToLibrary;
        }
        if (btn == QMessageBox::No) {
            return AddFilePolicy::KeepExternal;
        }
        return by_operation_default; // Cancel:按操作类型默认
    }
    default: // by_operation
        return by_operation_default;
    }
}

void PlaylistController::create_new_playlist()
{
    if (!manager_) {
        return;
    }
    manager_->create_playlist();
}

void PlaylistController::load_playlist()
{
    if (!manager_) {
        return;
    }

    QString path = QFileDialog::getOpenFileName(dialog_parent_, tr("Open Playlist File"), QString(),
                                                tr("WusicPlayer playlist (*.wcpl)"));
    if (!path.isEmpty()) {
        manager_->load_playlist(path);

        SortRule rule;
        rule.type = SortType::Album;
        manager_->get_view_model()->set_single_grouping(rule);
    }
}

void PlaylistController::rename_playlist(const PlaylistId& id)
{
    if (!manager_) {
        return;
    }

    PlaylistId target_id = id.is_null() ? manager_->get_current_playlist_id() : id;
    if (target_id.is_null()) {
        return;
    }
    QString old_name = manager_->get_playlist_by_id(target_id);

    bool ok          = false;
    QString new_name = QInputDialog::getText(dialog_parent_, tr("Rename Playlist"), tr("New name:"),
                                             QLineEdit::Normal, old_name, &ok);
    if (ok && !new_name.isEmpty()) {
        manager_->rename_playlist(target_id, new_name);
    }
}

void PlaylistController::rename_playlist(const PlaylistId& id, const QString& new_name)
{
    if (!manager_ || new_name.trimmed().isEmpty()) {
        return;
    }
    const PlaylistId target_id = id.is_null() ? manager_->get_current_playlist_id() : id;
    if (target_id.is_null()) {
        return;
    }
    manager_->rename_playlist(target_id, new_name.trimmed());
}

void PlaylistController::remove_playlist(const PlaylistId& id)
{
    if (!manager_) {
        return;
    }

    PlaylistId target_id = id.is_null() ? manager_->get_current_playlist_id() : id;
    if (target_id.is_null()) {
        return;
    }

    auto btn = QMessageBox::question(dialog_parent_, tr("Confirm Remove"),
                                     tr("Do you really want to remove this playlist?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        manager_->remove_playlist(target_id);
    }
}

void PlaylistController::save_playlist(const PlaylistId& id)
{
    if (!manager_) {
        return;
    }

    const PlaylistId target_id = id.is_null() ? manager_->get_current_playlist_id() : id;
    if (target_id.is_null()) {
        return;
    }

    QFileDialog dialog(dialog_parent_);
    dialog.setWindowTitle(tr("Save playlist file"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilters(QStringList() << tr("WusicPlayer playlist (*.wcpl)"));

    if (!dialog.exec()) {
        return;
    }

    QString filename = dialog.selectedFiles().first();
    if (!filename.endsWith(".wcpl", Qt::CaseInsensitive)) {
        filename += ".wcpl";
    }

    manager_->save_playlist(target_id, filename);
    logger->info("[PLAYLIST] playlist save to {}", filename);
}

void PlaylistController::copy_playlist(const PlaylistId& id)
{
    if (!manager_) {
        return;
    }

    const PlaylistId target_id = id.is_null() ? manager_->get_current_playlist_id() : id;
    if (!target_id.is_null()) {
        manager_->copy_playlist(target_id);
    }
}

void PlaylistController::remove_track(const EntryId& id)
{
    if (!manager_ || id.is_null()) {
        return;
    }

    auto btn = QMessageBox::question(dialog_parent_, tr("Confirm Remove"),
                                     tr("Do you really want to remove this track?"),
                                     QMessageBox::Yes | QMessageBox::No);

    if (btn == QMessageBox::Yes) {
        manager_->remove_track(id);
    }
}

int PlaylistController::add_library_tracks(const PlaylistId& pid, const QVector<TrackId>& track_ids)
{
    return manager_ ? manager_->add_library_tracks(pid, track_ids) : 0;
}

bool PlaylistController::locate_filepath(const QString& filepath)
{
    if (!manager_ || filepath.isEmpty()) {
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
    manager_->set_current_track(track->entry_id);
    return true;
}

void PlaylistController::remove_tracks(const QVector<EntryId>& ids)
{
    // 批量移除:不逐个弹窗确认(单选仍走带确认的 remove_track)
    if (!manager_) {
        return;
    }
    for (const EntryId& id : ids) {
        if (!id.is_null()) {
            manager_->remove_track(id);
        }
    }
}

int PlaylistController::copy_tracks_to_playlist(const PlaylistId& src_pid,
                                                const QVector<EntryId>& entry_ids,
                                                const PlaylistId& dst_pid)
{
    return manager_ ? manager_->copy_tracks_to_playlist(src_pid, entry_ids, dst_pid) : 0;
}

void PlaylistController::reorder_playlists(const QVector<PlaylistId>& ordered_ids)
{
    if (manager_) {
        manager_->reorder_playlists(ordered_ids);
    }
}

void PlaylistController::remove_missing_tracks()
{
    if (manager_) {
        manager_->remove_missing_tracks();
    }
}

auto PlaylistController::view_model() const
    -> decltype(std::declval<PlaylistManager*>()->get_view_model())
{
    return manager_->get_view_model();
}

EntryId PlaylistController::next_track() const
{
    return manager_->next_track(manager_->context_->get_play_mode());
}

EntryId PlaylistController::prev_track() const
{
    return manager_->prev_track(manager_->context_->get_play_mode());
}

QString PlaylistController::track_file_path(const EntryId& eid) const
{
    if (eid.is_null() || !manager_) {
        return {};
    }
    const PlaylistId pid = manager_->get_current_playlist_id();
    if (pid.is_null()) {
        return {};
    }
    const auto playlist = manager_->repo_->find_playlist_by_id(pid);
    if (!playlist) {
        return {};
    }
    const Track* track = playlist->find_track_by_id(eid);
    return track ? track->filepath : QString();
}

void PlaylistController::play(int queue_index)
{
    manager_->play(queue_index);
}

void PlaylistController::switch_to_playlist(const PlaylistId& id)
{
    manager_->switch_to_playlist(id);
}

void PlaylistController::set_play_mode(PlayMode mode)
{
    if (!manager_) {
        return;
    }
    manager_->context_->set_play_mode(mode);
}

PlayMode PlaylistController::play_mode() const
{
    return manager_ ? manager_->context_->get_play_mode() : PlayMode::InOrder;
}

QVector<std::shared_ptr<Playlist>> PlaylistController::playlists() const
{
    return manager_->get_playlists();
}
PlaylistId PlaylistController::current_playlist_id() const
{
    return manager_->get_current_playlist_id();
}
EntryId PlaylistController::current_track_id() const
{
    return manager_->get_current_track_id();
}
TrackMetaData PlaylistController::current_metadata() const
{
    return manager_->get_current_metadata();
}
std::shared_ptr<Playlist> PlaylistController::current_playlist()
{
    // return a share_ptr will cause the ref count increase
    return this->find_playlist_by_id(this->current_playlist_id());
}

std::shared_ptr<Playlist> PlaylistController::find_playlist_by_id(PlaylistId pid)
{
    if (pid.is_null()) {
        return nullptr;
    }
    return manager_->repo_->find_playlist_by_id(pid);
}

void PlaylistController::load_cache_after_shown()
{
    manager_->load_cache_after_shown();
}

void PlaylistController::set_group_rules(const QVector<SortRule>& rules)
{
    manager_->get_view_model()->set_group_rules(rules);
}

void PlaylistController::set_sort_rules(const QVector<SortRule>& rules)
{
    manager_->get_view_model()->set_sort_rules(rules);
}

QVector<SortRule> PlaylistController::group_rules() const
{
    return manager_->get_view_model()->group_rules();
}

QVector<SortRule> PlaylistController::sort_rules() const
{
    return manager_->get_view_model()->sort_rules();
}

void PlaylistController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj = json.value(this->config_sub_key()).toObject();

    QVector<TableColumn> columns;
    const QJsonArray column_arr = obj.value("columns").toArray();
    for (const auto& v : column_arr) {
        if (v.isObject()) {
            columns.append(json_to_column(v.toObject()));
        }
    }
    this->view_model()->set_columns(columns);

    QVector<SortRule> group_rules;
    const QJsonArray group_array = obj.value("group_rules").toArray();
    for (const auto& v : group_array) {
        if (v.isObject()) {
            group_rules.append(json_to_rule(v.toObject()));
        }
    }
    this->set_group_rules(group_rules);

    QVector<SortRule> sort_rules;
    QJsonArray sort_array = obj.value("sort_rules").toArray();
    for (const auto& v : sort_array) {
        if (v.isObject()) {
            sort_rules.append(json_to_rule(v.toObject()));
        }
    }
    this->set_sort_rules(sort_rules);

    // DSL 排序/分组/分类(存在则优先于上方 SortRule 规则)
    const QString dsl = obj.value("dsl_expression").toString();
    if (!dsl.trimmed().isEmpty()) {
        this->view_model()->set_sort_expression(dsl);
    }

    PlayMode mode =
        static_cast<PlayMode>(obj.value("play_mode").toInt(static_cast<int>(PlayMode::InOrder)));
    this->set_play_mode(mode);

    const AddFilePolicy policy = static_cast<AddFilePolicy>(
        obj.value("add_file_policy").toInt(static_cast<int>(AddFilePolicy::ByOperation)));
    this->set_add_file_policy(policy);

    last_playlist_id_ = PlaylistId(obj.value("last_playlist_id").toString());
    last_track_id_    = EntryId(obj.value("last_track_id").toString());

    if (!last_playlist_id_.is_null() && !this->find_playlist_by_id(last_playlist_id_)) {
        logger->warn("playlist {} does not exist!", last_playlist_id_.to_string());
    }
    // TODO: find track in current playlist if playlist available
}

QJsonObject PlaylistController::save_to_json()
{
    QJsonObject obj;
    QJsonArray column_array;
    for (const TableColumn& col : manager_->get_view_model()->get_columns()) {
        column_array.append(column_to_json(col));
    }
    obj["columns"] = column_array;

    QJsonArray group_array;
    for (const SortRule& rule : manager_->get_view_model()->group_rules()) {
        group_array.append(rule_to_json(rule));
    }
    obj["group_rules"] = group_array;

    QJsonArray sort_array;
    for (const SortRule& rule : manager_->get_view_model()->sort_rules()) {
        sort_array.append(rule_to_json(rule));
    }
    obj["sort_rules"]       = sort_array;
    obj["dsl_expression"]   = manager_->get_view_model()->dsl_source();

    obj["play_mode"]        = static_cast<int>(this->play_mode());
    obj["add_file_policy"]  = static_cast<int>(this->add_file_policy());

    last_playlist_id_       = this->current_playlist_id();
    last_track_id_          = this->current_track_id();

    obj["last_playlist_id"] = last_playlist_id_.to_string_without_brace();
    obj["last_track_id"]    = last_track_id_.to_string_without_brace();

    return obj;
}

QString PlaylistController::config_sub_key() const
{
    return "playlist";
}

PlaylistId PlaylistController::last_playlist_id() const
{
    return last_playlist_id_;
}

EntryId PlaylistController::last_track_id() const
{
    return last_track_id_;
}
