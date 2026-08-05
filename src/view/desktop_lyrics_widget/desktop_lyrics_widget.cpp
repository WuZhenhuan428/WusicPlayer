#include "view/desktop_lyrics_widget/desktop_lyrics_widget.h"

#include <QDebug>
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
    QWidget(parent), m_display_mode(DisplayMode::TwoLine), m_line_up_mode(AlignMode::Left),
    m_line_down_mode(AlignMode::Right), m_has_up_line_changed(false)
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

DesktopLyricsWidget::~DesktopLyricsWidget() {}

void DesktopLyricsWidget::init_ui()
{
    m_btn_shut_down = new QPushButton(this);
    m_btn_lock      = new QPushButton(this);
    m_btn_lock->setText("L");
    m_btn_lock->setFixedSize(20, 20);
    m_btn_shut_down->setText("X");
    m_btn_shut_down->setFixedSize(20, 20);

    m_hbl_toolbar = new QHBoxLayout();
    m_hbl_toolbar->setContentsMargins(0, 0, 0, 0);
    m_hbl_toolbar->addStretch();
    m_hbl_toolbar->addWidget(m_btn_lock);
    m_hbl_toolbar->addWidget(m_btn_shut_down);

    m_lrc_line_up   = new QLabel(this);
    m_lrc_line_down = new QLabel(this);
    m_lrc_line_up->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_lrc_line_down->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_lrc_line_up->setAlignment(Qt::AlignCenter);
    m_lrc_line_down->setAlignment(Qt::AlignCenter);

    m_hbl_lrc = new QVBoxLayout();
    m_hbl_lrc->setContentsMargins(0, 0, 0, 0);
    m_hbl_lrc->addSpacing(2);
    m_hbl_lrc->addWidget(m_lrc_line_up);
    m_hbl_lrc->addWidget(m_lrc_line_down);

    m_vbl_main = new QVBoxLayout();
    m_vbl_main->setContentsMargins(0, 0, 0, 0);
    m_vbl_main->setSpacing(6);
    m_vbl_main->addLayout(m_hbl_toolbar);
    m_vbl_main->addLayout(m_hbl_lrc);
    m_vbl_main->addStretch();

    this->setLayout(m_vbl_main);
}

void DesktopLyricsWidget::init_connect()
{
    connect(m_btn_shut_down, &QPushButton::clicked, this, &QWidget::hide);
    connect(m_btn_lock, &QPushButton::clicked, this, [this]() { set_locked(!m_is_locked); });
}

void DesktopLyricsWidget::set_locked(bool locked)
{
    if (m_is_locked == locked)
        return;
    m_is_locked = locked;
    m_btn_lock->setText(m_is_locked ? "U" : "L");
    m_btn_lock->setVisible(!m_is_locked);
    m_btn_shut_down->setVisible(!m_is_locked);
    apply_click_through(m_is_locked);
    if (!m_is_locked)
        update();
    emit sgnLockChanged(m_is_locked);
}

void DesktopLyricsWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_is_locked) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_drag_offset = event->globalPosition().toPoint() - pos();
        m_is_dragging = true;
        event->accept();
    }
}

void DesktopLyricsWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_is_dragging && !m_is_locked) {
        move(event->globalPosition().toPoint() - m_drag_offset);
        event->accept();
    }
}

void DesktopLyricsWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_is_dragging = false;
}

void DesktopLyricsWidget::paintEvent(QPaintEvent* event)
{
    if (!m_is_locked) {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 1));
    }
    QWidget::paintEvent(event);
}

void DesktopLyricsWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_btn_lock->setVisible(!m_is_locked);
    m_btn_shut_down->setVisible(!m_is_locked);
    apply_click_through(m_is_locked);
    m_is_visible = true;
    emit sgnVisibilityChanged(true);
}

void DesktopLyricsWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_is_visible = false;
    emit sgnVisibilityChanged(false);
}

void DesktopLyricsWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        update_line_color();
}

// ---- lyrics display ----

void DesktopLyricsWidget::set_lrc_line(const QString& curr_line, const QString& next_line)
{
    if (m_display_mode == DisplayMode::OneLine) {
        m_lrc_line_up->setText(curr_line);
        m_lrc_line_down->clear();
    } else if (m_display_mode == DisplayMode::TwoLine) {
        if (curr_line.isEmpty() && next_line.isEmpty()) {
            m_lrc_line_up->clear();
            m_lrc_line_down->clear();
            m_has_up_line_changed = true;
            return;
        }
        if (!curr_line.isEmpty() && next_line.isEmpty()) {
            m_lrc_line_up->setText(curr_line);
            m_lrc_line_down->clear();
            m_has_up_line_changed = false;
            return;
        }
        if (m_has_up_line_changed) {
            m_lrc_line_down->setText(curr_line);
            m_lrc_line_up->setText(next_line);
        } else {
            m_lrc_line_up->setText(curr_line);
            m_lrc_line_down->setText(next_line);
        }
        m_has_up_line_changed = !m_has_up_line_changed;
    }
}

void DesktopLyricsWidget::set_lrc_font(QFont font)
{
    m_font = font;
    m_lrc_line_up->setFont(m_font);
    m_lrc_line_down->setFont(m_font);
    QFontMetrics fm(m_font);
    int lh = fm.height();
    m_lrc_line_up->setFixedHeight(lh);
    m_lrc_line_down->setFixedHeight(lh);
    int lines = (m_display_mode == DisplayMode::TwoLine) ? 2 : 1;
    int barH = 24, sp = m_vbl_main->spacing() + m_hbl_lrc->spacing();
    QMargins mg = m_vbl_main->contentsMargins();
    setFixedHeight(mg.top() + barH + (lines * lh) + sp + mg.bottom());
}

QFont DesktopLyricsWidget::get_font()
{
    return m_font;
}

void DesktopLyricsWidget::update_line_color()
{
    QPalette pe_a;
    pe_a.setColor(QPalette::WindowText, QColor(m_rgb_active.r, m_rgb_active.g, m_rgb_active.b));
    if (m_display_mode == DisplayMode::OneLine) {
        m_lrc_line_up->setPalette(pe_a);
        m_lrc_line_down->setPalette(pe_a);
    } else {
        QPalette pe_i;
        pe_i.setColor(QPalette::WindowText,
                      QColor(m_rgb_inactive.r, m_rgb_inactive.g, m_rgb_inactive.b));
        if (m_has_up_line_changed) {
            m_lrc_line_up->setPalette(pe_a);
            m_lrc_line_down->setPalette(pe_i);
        } else {
            m_lrc_line_up->setPalette(pe_i);
            m_lrc_line_down->setPalette(pe_a);
        }
    }
}

void DesktopLyricsWidget::set_active_line_color(rgb_t c)
{
    m_rgb_active = c;
    update_line_color();
}
void DesktopLyricsWidget::set_inactive_line_color(rgb_t c)
{
    m_rgb_inactive = c;
    update_line_color();
}
rgb_t DesktopLyricsWidget::get_active_line_color()
{
    return m_rgb_active;
}
rgb_t DesktopLyricsWidget::get_inactive_line_color()
{
    return m_rgb_inactive;
}

void DesktopLyricsWidget::set_display_mode(DisplayMode d)
{
    m_display_mode = d;
    apply_config();
}
void DesktopLyricsWidget::set_up_line_align_mode(AlignMode a)
{
    m_line_up_mode = a;
    apply_config();
}
void DesktopLyricsWidget::set_down_line_align_mode(AlignMode a)
{
    m_line_down_mode = a;
    apply_config();
}

void DesktopLyricsWidget::apply_config()
{
    if (m_display_mode == DisplayMode::OneLine)
        m_lrc_line_down->hide();
    else
        m_lrc_line_down->show();
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
    m_lrc_line_up->setAlignment(al(m_line_up_mode));
    m_lrc_line_down->setAlignment(al(m_line_down_mode));
    set_active_line_color(m_rgb_active);
    set_inactive_line_color(m_rgb_inactive);
}

const QByteArray DesktopLyricsWidget::get_geometry() const
{
    return saveGeometry();
}

void DesktopLyricsWidget::setGeometry(const QByteArray& geo)
{
    if (geo.isEmpty())
        return;
    restoreGeometry(geo);
    QScreen* s = QGuiApplication::screenAt(pos());
    if (!s)
        s = QGuiApplication::primaryScreen();
    if (!s->availableGeometry().contains(rect()))
        move(s->availableGeometry().center() - rect().center());
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
    m_is_visible     = o.value("is_visible").toBool(true);
    m_is_locked      = o.value("is_locked").toBool(false);
    m_rgb_active.r   = o.value("rgb_active_r").toInt();
    m_rgb_active.g   = o.value("rgb_active_g").toInt();
    m_rgb_active.b   = o.value("rgb_active_b").toInt();
    m_rgb_inactive.r = o.value("rgb_inactive_r").toInt();
    m_rgb_inactive.g = o.value("rgb_inactive_g").toInt();
    m_rgb_inactive.b = o.value("rgb_inactive_b").toInt();
    m_font.fromString(o.value("font_string").toString());
    set_lrc_font(m_font);
    m_btn_lock->setText(m_is_locked ? "U" : "L");
    m_btn_lock->setVisible(!m_is_locked);
    m_btn_shut_down->setVisible(!m_is_locked);
    if (m_is_visible)
        QTimer::singleShot(20, this, &QWidget::show);
    else
        hide();
}

QJsonObject DesktopLyricsWidget::save_to_json()
{
    QJsonObject o;
    o["geometry"]       = QString::fromUtf8(get_geometry().toBase64());
    o["is_visible"]     = m_is_visible;
    o["is_locked"]      = m_is_locked;
    o["rgb_active_r"]   = m_rgb_active.r;
    o["rgb_active_g"]   = m_rgb_active.g;
    o["rgb_active_b"]   = m_rgb_active.b;
    o["rgb_inactive_r"] = m_rgb_inactive.r;
    o["rgb_inactive_g"] = m_rgb_inactive.g;
    o["rgb_inactive_b"] = m_rgb_inactive.b;
    o["font_string"]    = m_font.toString();
    return o;
}

QString DesktopLyricsWidget::config_sub_key() const
{
    return "desktop_lyrics";
}
