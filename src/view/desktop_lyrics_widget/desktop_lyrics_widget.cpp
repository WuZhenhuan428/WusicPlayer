#include "view/desktop_lyrics_widget/desktop_lyrics_widget.h"

#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonObject>
#include <QMargins>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#ifdef Q_OS_WIN
#    include <dwmapi.h>
#    include <windows.h>
#endif

DesktopLyricsWidget::DesktopLyricsWidget(QWidget* parent) :
    QWidget(parent), display_mode_(DisplayMode::TwoLine), line_up_mode_(AlignMode::Left),
    line_down_mode_(AlignMode::Right), has_up_line_changed_(false)
{
    this->init_ui();
    this->setFixedSize(700, 120);

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
#ifdef Q_OS_WIN
                         | Qt::Tool // Windows: hide from taskbar
#else
                         | Qt::Window
#endif
    );
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    apply_config();
    this->init_connect();
}

void DesktopLyricsWidget::init_ui()
{
    btn_shut_down_ = new QPushButton(this);
    btn_lock_      = new QPushButton(this);
    btn_lock_->setText("L");
    btn_lock_->setFixedSize(20, 20);
    btn_shut_down_->setText("X");
    btn_shut_down_->setFixedSize(20, 20);

    hbl_toolbar_ = new QHBoxLayout();
    hbl_toolbar_->setContentsMargins(0, 0, 0, 0);
    hbl_toolbar_->addStretch();
    hbl_toolbar_->addWidget(btn_lock_);
    hbl_toolbar_->addWidget(btn_shut_down_);

    lrc_line_up_   = new QLabel(this);
    lrc_line_down_ = new QLabel(this);
    lrc_line_up_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lrc_line_down_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lrc_line_up_->setAlignment(Qt::AlignCenter);
    lrc_line_down_->setAlignment(Qt::AlignCenter);

    hbl_lrc_ = new QVBoxLayout();
    hbl_lrc_->setContentsMargins(0, 0, 0, 0);
    hbl_lrc_->addSpacing(2);
    hbl_lrc_->addWidget(lrc_line_up_);
    hbl_lrc_->addWidget(lrc_line_down_);

    vbl_main_ = new QVBoxLayout();
    vbl_main_->setContentsMargins(0, 0, 0, 0);
    vbl_main_->setSpacing(6);
    vbl_main_->addLayout(hbl_toolbar_);
    vbl_main_->addLayout(hbl_lrc_);
    vbl_main_->addStretch();

    this->setLayout(vbl_main_);
}

void DesktopLyricsWidget::init_connect()
{
    connect(btn_shut_down_, &QPushButton::clicked, this, &QWidget::hide);
    connect(btn_lock_, &QPushButton::clicked, this, [this]() { set_locked(!is_locked_); });
}

void DesktopLyricsWidget::set_locked(bool locked)
{
    if (is_locked_ == locked) {
        return;
    }
    is_locked_ = locked;
    btn_lock_->setText(is_locked_ ? "U" : "L");
    btn_lock_->setVisible(!is_locked_);
    btn_shut_down_->setVisible(!is_locked_);
    apply_click_through(is_locked_);
    if (!is_locked_) {
        update();
    }
    emit sgn_lock_changed(is_locked_);
}

void DesktopLyricsWidget::mousePressEvent(QMouseEvent* event)
{
    if (is_locked_) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        drag_offset_ = event->globalPosition().toPoint() - pos();
        is_dragging_ = true;
        event->accept();
    }
}

void DesktopLyricsWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (is_dragging_ && !is_locked_) {
        move(event->globalPosition().toPoint() - drag_offset_);
        event->accept();
    }
}

void DesktopLyricsWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        is_dragging_ = false;
    }
}

void DesktopLyricsWidget::paintEvent(QPaintEvent* event)
{
    if (!is_locked_) {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 1));
    }
    QWidget::paintEvent(event);
}

void DesktopLyricsWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    btn_lock_->setVisible(!is_locked_);
    btn_shut_down_->setVisible(!is_locked_);
    apply_click_through(is_locked_);
    is_visible_ = true;
    emit sgn_visibility_changed(true);
}

void DesktopLyricsWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    is_visible_ = false;
    emit sgn_visibility_changed(false);
}

void DesktopLyricsWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        update_line_color();
    }
}

// ---- lyrics display ----

void DesktopLyricsWidget::set_lrc_line(const QString& curr_line, const QString& next_line)
{
    if (display_mode_ == DisplayMode::OneLine) {
        lrc_line_up_->setText(curr_line);
        lrc_line_down_->clear();
    } else if (display_mode_ == DisplayMode::TwoLine) {
        if (curr_line.isEmpty() && next_line.isEmpty()) {
            lrc_line_up_->clear();
            lrc_line_down_->clear();
            has_up_line_changed_ = true;
            return;
        }
        if (!curr_line.isEmpty() && next_line.isEmpty()) {
            lrc_line_up_->setText(curr_line);
            lrc_line_down_->clear();
            has_up_line_changed_ = false;
            return;
        }
        if (has_up_line_changed_) {
            lrc_line_down_->setText(curr_line);
            lrc_line_up_->setText(next_line);
        } else {
            lrc_line_up_->setText(curr_line);
            lrc_line_down_->setText(next_line);
        }
        has_up_line_changed_ = !has_up_line_changed_;
    }
}

void DesktopLyricsWidget::set_lrc_font(QFont font)
{
    font_ = std::move(font);
    lrc_line_up_->setFont(font_);
    lrc_line_down_->setFont(font_);
    QFontMetrics fm(font_);
    int lh = fm.height();
    lrc_line_up_->setFixedHeight(lh);
    lrc_line_down_->setFixedHeight(lh);
    int lines   = (display_mode_ == DisplayMode::TwoLine) ? 2 : 1;
    int bar_h   = 24;
    int sp      = vbl_main_->spacing() + hbl_lrc_->spacing();
    QMargins mg = vbl_main_->contentsMargins();
    setFixedHeight(mg.top() + bar_h + (lines * lh) + sp + mg.bottom());
}

QFont DesktopLyricsWidget::get_font()
{
    return font_;
}

void DesktopLyricsWidget::update_line_color()
{
    QPalette pe_a;
    pe_a.setColor(QPalette::WindowText, QColor(rgb_active_.r, rgb_active_.g, rgb_active_.b));
    if (display_mode_ == DisplayMode::OneLine) {
        lrc_line_up_->setPalette(pe_a);
        lrc_line_down_->setPalette(pe_a);
    } else {
        QPalette pe_i;
        pe_i.setColor(QPalette::WindowText,
                      QColor(rgb_inactive_.r, rgb_inactive_.g, rgb_inactive_.b));
        if (has_up_line_changed_) {
            lrc_line_up_->setPalette(pe_a);
            lrc_line_down_->setPalette(pe_i);
        } else {
            lrc_line_up_->setPalette(pe_i);
            lrc_line_down_->setPalette(pe_a);
        }
    }
}

void DesktopLyricsWidget::set_active_line_color(rgb_t r)
{
    rgb_active_ = r;
    update_line_color();
}
void DesktopLyricsWidget::set_inactive_line_color(rgb_t r)
{
    rgb_inactive_ = r;
    update_line_color();
}
rgb_t DesktopLyricsWidget::get_active_line_color()
{
    return rgb_active_;
}
rgb_t DesktopLyricsWidget::get_inactive_line_color()
{
    return rgb_inactive_;
}

void DesktopLyricsWidget::set_display_mode(DisplayMode d)
{
    display_mode_ = d;
    apply_config();
}
void DesktopLyricsWidget::set_up_line_align_mode(AlignMode a)
{
    line_up_mode_ = a;
    apply_config();
}
void DesktopLyricsWidget::set_down_line_align_mode(AlignMode a)
{
    line_down_mode_ = a;
    apply_config();
}

void DesktopLyricsWidget::apply_config()
{
    if (display_mode_ == DisplayMode::OneLine) {
        lrc_line_down_->hide();
    } else {
        lrc_line_down_->show();
    }
    auto al = [](AlignMode m) {
        switch (m) {
        case AlignMode::Left:
            return Qt::AlignLeft;
        case AlignMode::Middle:
            return Qt::AlignCenter;
        case AlignMode::Right:
            return Qt::AlignRight;
        }
        return Qt::AlignCenter;
    };
    lrc_line_up_->setAlignment(al(line_up_mode_));
    lrc_line_down_->setAlignment(al(line_down_mode_));
    set_active_line_color(rgb_active_);
    set_inactive_line_color(rgb_inactive_);
}

QByteArray DesktopLyricsWidget::get_geometry() const
{
    return saveGeometry();
}

void DesktopLyricsWidget::setGeometry(const QByteArray& geo)
{
    if (geo.isEmpty()) {
        return;
    }
    restoreGeometry(geo);
    QScreen* s = QGuiApplication::screenAt(pos());
    if (!s) {
        s = QGuiApplication::primaryScreen();
    }
    if (!s->availableGeometry().contains(rect())) {
        move(s->availableGeometry().center() - rect().center());
    }
}

void DesktopLyricsWidget::apply_click_through([[maybe_unused]] bool on)
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd)
        return;
    BOOL ncrp = on ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ex |= WS_EX_LAYERED;
    if (on)
        ex |= WS_EX_TRANSPARENT;
    else
        ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
    // do nothing on linux
#endif
}

// ---- config ----

void DesktopLyricsWidget::load_from_json(const QJsonObject& json)
{
    QJsonObject o = json.value(config_sub_key()).toObject();
    restoreGeometry(QByteArray::fromBase64(o.value("geometry").toString().toUtf8()));
    is_visible_     = o.value("is_visible").toBool(true);
    is_locked_      = o.value("is_locked").toBool(false);
    rgb_active_.r   = o.value("rgb_active_r").toInt();
    rgb_active_.g   = o.value("rgb_active_g").toInt();
    rgb_active_.b   = o.value("rgb_active_b").toInt();
    rgb_inactive_.r = o.value("rgb_inactive_r").toInt();
    rgb_inactive_.g = o.value("rgb_inactive_g").toInt();
    rgb_inactive_.b = o.value("rgb_inactive_b").toInt();
    font_.fromString(o.value("font_string").toString());
    set_lrc_font(font_);
    btn_lock_->setText(is_locked_ ? "U" : "L");
    btn_lock_->setVisible(!is_locked_);
    btn_shut_down_->setVisible(!is_locked_);
    if (is_visible_) {
        QTimer::singleShot(20, this, &QWidget::show);
    } else {
        hide();
    }
}

QJsonObject DesktopLyricsWidget::save_to_json()
{
    QJsonObject o;
    o["geometry"]       = QString::fromUtf8(get_geometry().toBase64());
    o["is_visible"]     = is_visible_;
    o["is_locked"]      = is_locked_;
    o["rgb_active_r"]   = rgb_active_.r;
    o["rgb_active_g"]   = rgb_active_.g;
    o["rgb_active_b"]   = rgb_active_.b;
    o["rgb_inactive_r"] = rgb_inactive_.r;
    o["rgb_inactive_g"] = rgb_inactive_.g;
    o["rgb_inactive_b"] = rgb_inactive_.b;
    o["font_string"]    = font_.toString();
    return o;
}

QString DesktopLyricsWidget::config_sub_key() const
{
    return "desktop_lyrics";
}
