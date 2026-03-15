#include "DesktopLyricsWidget.h"

#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QFontMetrics>
#include <QMargins>

DesktopLyricsWidget::DesktopLyricsWidget(QWidget* parent)
    : QWidget(parent),
      m_isLocked(false),
      m_displayMode(DisplayMode::TwoLine),
      m_lineUpMode(AlignMode::Left),
      m_lineDownMode(AlignMode::Right),
      m_has_up_line_changed(false)

{
    this->initUI();
    this->resize(700, 120);
    
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_TransparentForMouseEvents);

    QFont font_test;
    font_test.setFamily("JetBrains Maple Mono");
    font_test.setPixelSize(30);
    setLrcFont(font_test);
    applyConfig();

    m_lrcLineUp->setStyleSheet("color:#00A3CC");
    m_lrcLineDown->setStyleSheet("color:#00A3CC");

    this->initConnect();
}

DesktopLyricsWidget::~DesktopLyricsWidget() {}

void DesktopLyricsWidget::initUI() {
    m_btnShutDown = new QPushButton(this);
    m_btnLock = new QPushButton(this);

    m_btnLock->setText("L");
    m_btnLock->setFixedSize(20, 20);
    m_btnShutDown->setText("X");
    m_btnShutDown->setFixedSize(20, 20);
    
    m_toolBarLayout = new QHBoxLayout();
    m_toolBarLayout->setContentsMargins(0, 0, 0, 0);
    m_toolBarLayout->addStretch();
    m_toolBarLayout->addWidget(m_btnLock);
    m_toolBarLayout->addWidget(m_btnShutDown);
    
    m_lrcLineUp = new QLabel(this);
    m_lrcLineDown = new QLabel(this);
    
    m_lrcLineUp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_lrcLineDown->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_lrcLineUp->setAlignment(Qt::AlignCenter);
    m_lrcLineDown->setAlignment(Qt::AlignCenter);

    m_lrcLineLayout = new QVBoxLayout();
    m_lrcLineLayout->setContentsMargins(0, 0, 0, 0);
    m_lrcLineLayout->addSpacing(2);
    m_lrcLineLayout->addWidget(m_lrcLineUp);
    m_lrcLineLayout->addWidget(m_lrcLineDown);
    
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(6);
    m_mainLayout->addLayout(m_toolBarLayout);
    m_mainLayout->addLayout(m_lrcLineLayout);
    m_mainLayout->addStretch();
    
    this->setLayout(m_mainLayout);
}

void DesktopLyricsWidget::initConnect() {
    connect(m_btnShutDown, &QPushButton::clicked, this, &QWidget::hide);
    connect(m_btnLock, &QPushButton::clicked, this, [this](){
        m_isLocked = !m_isLocked;
        if (m_isLocked) {
            m_btnLock->setText("U");    // need unlock
        } else if (!m_isLocked) {
            m_btnLock->setText("L");
        }
    });
}


void DesktopLyricsWidget::setLrcLine(const QString& curr_line, const QString& next_line) {
    if (m_displayMode == DisplayMode::OneLine) {
        m_lrcLineUp->setText(curr_line);
        m_lrcLineDown->clear();
    } else if (m_displayMode == DisplayMode::TwoLine) {
        if (m_has_up_line_changed) {
            m_lrcLineDown->setText(curr_line);
            m_lrcLineUp->setText(next_line);
        } else if (!m_has_up_line_changed) {
            m_lrcLineUp->setText(curr_line);
            m_lrcLineDown->setText(next_line);
        }
        m_has_up_line_changed = !m_has_up_line_changed;
    }
}

void DesktopLyricsWidget::setLrcFont(QFont font) {
    m_lrcLineUp->setFont(font);
    m_lrcLineDown->setFont(font);

    QFontMetrics fm(font);
    const int line_height = fm.height();
    m_lrcLineUp->setFixedHeight(line_height);
    m_lrcLineDown->setFixedHeight(line_height);

    const int lines = (m_displayMode == DisplayMode::TwoLine) ? 2 : 1;
    const int toolbar_height = 24;
    const int spacing = m_mainLayout->spacing() + m_lrcLineLayout->spacing();
    const QMargins mg = m_mainLayout->contentsMargins();
    const int height = mg.top() + toolbar_height + (lines * line_height) + spacing + mg.bottom();
    setFixedHeight(height);
}


void DesktopLyricsWidget::mousePressEvent(QMouseEvent* event) {
    if (m_isLocked) {
        event->ignore();
    } else if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
        event->accept();
    }
}

void DesktopLyricsWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isLocked) {
        event->ignore();
    } else if (event->buttons() & Qt::LeftButton) {
        this->move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
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


void DesktopLyricsWidget::setDisplayMode(DisplayMode disp_mode, AlignMode line_up_mode, AlignMode line_down_mode) {
    m_displayMode = disp_mode;
    m_lineUpMode = line_up_mode;
    m_lineDownMode = line_down_mode;
    this->applyConfig();
}

void DesktopLyricsWidget::applyConfig() {
    if (m_displayMode == DisplayMode::OneLine) {
        m_lrcLineDown->hide();
    } else if (m_displayMode == DisplayMode::TwoLine) {
        m_lrcLineDown->show();
    }

    if (m_lineUpMode == AlignMode::Left) {
        m_lrcLineUp->setAlignment(Qt::AlignLeft);
    } else if (m_lineUpMode == AlignMode::Middle) {
        m_lrcLineUp->setAlignment(Qt::AlignCenter);
    } else if (m_lineUpMode == AlignMode::Right) {
        m_lrcLineUp->setAlignment(Qt::AlignRight);
    }

    if (m_lineDownMode == AlignMode::Left) {
        m_lrcLineDown->setAlignment(Qt::AlignLeft);
    } else if (m_lineDownMode == AlignMode::Middle) {
        m_lrcLineDown->setAlignment(Qt::AlignCenter);
    } else if (m_lineDownMode == AlignMode::Right) {
        m_lrcLineDown->setAlignment(Qt::AlignRight);
    }
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
