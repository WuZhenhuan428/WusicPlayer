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
    ~DesktopLyricsWidget();

    void set_lrc_line(const QString& curr_line, const QString& next_line = QString());
    void set_lrc_font(QFont font);
    QFont get_font();
    void set_display_mode(DisplayMode disp_mode);
    void set_up_line_align_mode(AlignMode line_up_mode);
    void set_down_line_align_mode(AlignMode line_down_mode);
    void apply_config();
    const QByteArray get_geometry() const;
    void setGeometry(const QByteArray& geo);
    void update_line_color();
    void set_active_line_color(rgb_t rgb_active);
    void set_inactive_line_color(rgb_t rgb_inactive);
    rgb_t get_active_line_color();
    rgb_t get_inactive_line_color();
    bool isLocked() const
    {
        return m_is_locked;
    }

public slots:
    void set_locked(bool locked);

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgnVisibilityChanged(bool visible);
    void sgnLockChanged(bool locked);

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
    void apply_click_through(bool clickThrough);

    bool m_is_locked   = false;
    bool m_is_dragging = false;
    QPoint m_drag_offset;
    QFont m_font;

    DisplayMode m_display_mode;
    AlignMode m_line_up_mode;
    AlignMode m_line_down_mode;
    bool m_has_up_line_changed;

    QPushButton* m_btn_lock      = nullptr;
    QPushButton* m_btn_shut_down = nullptr;

    QLabel* m_lrc_line_up        = nullptr;
    QLabel* m_lrc_line_down      = nullptr;

    QHBoxLayout* m_hbl_toolbar   = nullptr;
    QVBoxLayout* m_hbl_lrc       = nullptr;
    QVBoxLayout* m_vbl_main      = nullptr;

    rgb_t m_rgb_active;
    rgb_t m_rgb_inactive;

    bool m_is_visible = false;
};
