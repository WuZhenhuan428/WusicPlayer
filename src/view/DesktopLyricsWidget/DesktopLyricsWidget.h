#pragma once

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>
#include <QFont>
#include <QMouseEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QPoint>
#include <QVector>
#include <QByteArray>
#include "core/types.h"
#include "core/hsv_types.h"

class DesktopLyricsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DesktopLyricsWidget(QWidget* parent = nullptr);
    ~DesktopLyricsWidget();

    void setLrcLine(const QString& curr_line, const QString& next_line = QString());
    void setLrcFont(QFont font);
    void setDisplayMode(DisplayMode disp_mode);
    void setUpLineAlignMode(AlignMode line_up_mode);
    void setDownLineAlignMode(AlignMode line_down_mode);
    void applyConfig();
    const QByteArray getGeometry() const;
    void setGeometry(const QByteArray& geo);
    void updateLineColor();
    void setActiveLineColor(rgb_t rgb_active);
    void setInactiveLineColor(rgb_t rgb_inactive);
    rgb_t getActiveLineColor();
    rgb_t getInactiveLineColor();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

signals:
    void sgnVisibilityChanged(bool visible);
private:

    void initUI();
    void initConnect();

    QPoint m_dragPosition;
    bool m_isLocked;
    QFont m_font;

    DisplayMode m_displayMode;
    AlignMode m_lineUpMode;
    AlignMode m_lineDownMode;
    bool m_has_up_line_changed;


    QPushButton* m_btnLock = nullptr;
    QPushButton* m_btnShutDown = nullptr;

    QLabel* m_lrcLineUp = nullptr;
    QLabel* m_lrcLineDown = nullptr;

    QHBoxLayout* m_toolBarLayout = nullptr;
    QVBoxLayout* m_lrcLineLayout = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    rgb_t m_rgb_active;
    rgb_t m_rgb_inactive;
};
