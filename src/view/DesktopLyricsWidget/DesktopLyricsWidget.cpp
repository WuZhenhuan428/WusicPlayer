#include "DesktopLyricsWidget.h"

#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QFontMetrics>
#include <QMargins>
#include <QWindow>

DesktopLyricsWidget::DesktopLyricsWidget(QWidget* parent)
    : QWidget(parent),
      m_is_locked(false),
      m_display_mode(DisplayMode::TwoLine),
      m_line_up_mode(AlignMode::Left),
      m_line_down_mode(AlignMode::Right),
      m_has_up_line_changed(false)
{
    this->initUI();
    this->setFixedSize(700, 120);
    
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_TransparentForMouseEvents);

    QFont font_test;
    font_test.setFamily("JetBrains Maple Mono");
    font_test.setPixelSize(30);
    setLrcFont(font_test);
    applyConfig();

    this->initConnect();
}

DesktopLyricsWidget::~DesktopLyricsWidget() {}

void DesktopLyricsWidget::initUI() {
    m_btn_shut_down = new QPushButton(this);
    m_btn_lock = new QPushButton(this);

    m_btn_lock->setText("L");
    m_btn_lock->setFixedSize(20, 20);
    m_btn_shut_down->setText("X");
    m_btn_shut_down->setFixedSize(20, 20);
    
    m_hbl_toolbar = new QHBoxLayout();
    m_hbl_toolbar->setContentsMargins(0, 0, 0, 0);
    m_hbl_toolbar->addStretch();
    m_hbl_toolbar->addWidget(m_btn_lock);
    m_hbl_toolbar->addWidget(m_btn_shut_down);
    
    m_lrc_line_up = new QLabel(this);
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

void DesktopLyricsWidget::initConnect() {
    connect(m_btn_shut_down, &QPushButton::clicked, this, &QWidget::hide);
    connect(m_btn_lock, &QPushButton::clicked, this, [this](){
        m_is_locked = !m_is_locked;
        if (m_is_locked) {
            m_btn_lock->setText("U");    // need Unlock
        } else if (!m_is_locked) {
            m_btn_lock->setText("L");
        }
    });
}

void DesktopLyricsWidget::updateLineColor() {
    QPalette pe_active;
    pe_active.setColor(QPalette::WindowText, QColor(m_rgb_active.r, m_rgb_active.g, m_rgb_active.b));
    if (m_display_mode == DisplayMode::OneLine) {
        m_lrc_line_up->setPalette(pe_active);
        m_lrc_line_down->setPalette(pe_active);
    }
    if (m_display_mode == DisplayMode::TwoLine) {
        QPalette pe_inactive;
        pe_inactive.setColor(QPalette::WindowText, QColor(m_rgb_inactive.r, m_rgb_inactive.g, m_rgb_inactive.b));
        if (m_has_up_line_changed) {
            m_lrc_line_up->setPalette(pe_active);
            m_lrc_line_down->setPalette(pe_inactive);
        } else {
            m_lrc_line_up->setPalette(pe_inactive);
            m_lrc_line_down->setPalette(pe_active);
        }
    }
}

void DesktopLyricsWidget::setActiveLineColor(rgb_t rgb_active) {
    m_rgb_active = rgb_active;
    this->updateLineColor();
}

void DesktopLyricsWidget::setInactiveLineColor(rgb_t rgb_inactive) {
    m_rgb_inactive = rgb_inactive;
    this->updateLineColor();
}

rgb_t DesktopLyricsWidget::getActiveLineColor() {
    return m_rgb_active;
}
rgb_t DesktopLyricsWidget::getInactiveLineColor() {
    return m_rgb_inactive;
}

void DesktopLyricsWidget::setLrcLine(const QString& curr_line, const QString& next_line) {
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
        if (m_has_up_line_changed) {
            m_lrc_line_down->setText(curr_line);
            m_lrc_line_up->setText(next_line);
        } else if (!m_has_up_line_changed) {
            m_lrc_line_up->setText(curr_line);
            m_lrc_line_down->setText(next_line);
        }
        m_has_up_line_changed = !m_has_up_line_changed;
    }
}

void DesktopLyricsWidget::setLrcFont(QFont font) {
    m_font = font;
    m_lrc_line_up->setFont(m_font);
    m_lrc_line_down->setFont(m_font);

    QFontMetrics fm(m_font);
    const int line_height = fm.height();
    m_lrc_line_up->setFixedHeight(line_height);
    m_lrc_line_down->setFixedHeight(line_height);

    const int lines = (m_display_mode == DisplayMode::TwoLine) ? 2 : 1;
    const int toolbar_height = 24;
    const int spacing = m_vbl_main->spacing() + m_hbl_lrc->spacing();
    const QMargins mg = m_vbl_main->contentsMargins();
    const int height = mg.top() + toolbar_height + (lines * line_height) + spacing + mg.bottom();
    setFixedHeight(height);
}

QFont DesktopLyricsWidget::getFont() {
    return m_font;
}


void DesktopLyricsWidget::mousePressEvent(QMouseEvent* event) {
    if (m_is_locked) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        if (windowHandle()) {
            windowHandle()->startSystemMove();
        }
    }
}

void DesktopLyricsWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    emit sgnVisibilityChanged(true);
}

void DesktopLyricsWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    emit sgnVisibilityChanged(false);
}


void DesktopLyricsWidget::setDisplayMode(DisplayMode disp_mode) {
    m_display_mode = disp_mode;
    this->applyConfig();
}

void DesktopLyricsWidget::setUpLineAlignMode(AlignMode line_up_mode) {
    m_line_up_mode = line_up_mode;
    this->applyConfig();
}

void DesktopLyricsWidget::setDownLineAlignMode(AlignMode line_down_mode) {
    m_line_down_mode = line_down_mode;
    this->applyConfig();
}


void DesktopLyricsWidget::applyConfig() {
    if (m_display_mode == DisplayMode::OneLine) {
        m_lrc_line_down->hide();
    } else if (m_display_mode == DisplayMode::TwoLine) {
        m_lrc_line_down->show();
    }

    if (m_line_up_mode == AlignMode::Left) {
        m_lrc_line_up->setAlignment(Qt::AlignLeft);
    } else if (m_line_up_mode == AlignMode::Middle) {
        m_lrc_line_up->setAlignment(Qt::AlignCenter);
    } else if (m_line_up_mode == AlignMode::Right) {
        m_lrc_line_up->setAlignment(Qt::AlignRight);
    }

    if (m_line_down_mode == AlignMode::Left) {
        m_lrc_line_down->setAlignment(Qt::AlignLeft);
    } else if (m_line_down_mode == AlignMode::Middle) {
        m_lrc_line_down->setAlignment(Qt::AlignCenter);
    } else if (m_line_down_mode == AlignMode::Right) {
        m_lrc_line_down->setAlignment(Qt::AlignRight);
    }

    this->setActiveLineColor(m_rgb_active);
    this->setInactiveLineColor(m_rgb_inactive);                                     
}

const QByteArray DesktopLyricsWidget::getGeometry() const {
    return this->saveGeometry();
}

void DesktopLyricsWidget::setGeometry(const QByteArray& geo) {
    if (geo.isEmpty()) return;
    this->restoreGeometry(geo);
    QScreen *screen = QGuiApplication::screenAt(this->pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen->availableGeometry().contains(this->rect())) {
        move(screen->availableGeometry().center() - rect().center());
    }
}
