#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/hsv_types.h"
#include "core/types.h"

#include <QByteArray>
#include <QFont>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QPushButton>
#include <QShowEvent>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class DesktopLyricsWidget : public QWidget, public IConfigurable
{
    Q_OBJECT
public:
    explicit DesktopLyricsWidget(QWidget* parent = nullptr);

    void set_lrc_line(const QString& curr_line, const QString& next_line = QString());
    void set_lrc_font(QFont font);
    QFont get_font();
    void set_display_mode(DisplayMode disp_mode);
    void set_up_line_align_mode(AlignMode a);
    void set_down_line_align_mode(AlignMode a);
    void apply_config();
    [[nodiscard]] QByteArray get_geometry() const;
    void setGeometry(const QByteArray& geo);
    void update_line_color();
    void set_active_line_color(rgb_t r);
    void set_inactive_line_color(rgb_t r);
    rgb_t get_active_line_color();
    rgb_t get_inactive_line_color();
    [[nodiscard]] bool is_locked() const
    {
        return is_locked_;
    }

public slots:
    void set_locked(bool locked);

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override;

signals:
    void sgn_visibility_changed(bool visible);
    void sgn_lock_changed(bool locked);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void init_ui();
    void init_connect();
    void apply_click_through(bool on);

    bool is_locked_   = false;
    bool is_dragging_ = false;
    QPoint drag_offset_;
    QFont font_;

    DisplayMode display_mode_;
    AlignMode line_up_mode_;
    AlignMode line_down_mode_;
    bool has_up_line_changed_;

    QPushButton* btn_lock_      = nullptr;
    QPushButton* btn_shut_down_ = nullptr;

    QLabel* lrc_line_up_        = nullptr;
    QLabel* lrc_line_down_      = nullptr;

    QHBoxLayout* hbl_toolbar_   = nullptr;
    QVBoxLayout* hbl_lrc_       = nullptr;
    QVBoxLayout* vbl_main_      = nullptr;

    rgb_t rgb_active_           = {};
    rgb_t rgb_inactive_         = {};

    bool is_visible_            = false;
};
