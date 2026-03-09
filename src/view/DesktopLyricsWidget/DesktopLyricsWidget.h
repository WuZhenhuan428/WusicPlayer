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
#include <QPoint>
#include <QVector>
#include <QByteArray>
#include "core/types.h"

class DesktopLyricsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DesktopLyricsWidget(QWidget* parent = nullptr);
    ~DesktopLyricsWidget();

    void setLrcLine(const QString& curr_line, const QString& next_line = QString());
    void setLrcFont(QFont font);
    void setDisplayMode(DisplayMode disp_mode, AlignMode line_up_mode, AlignMode line_down_mode);
    void applyConfig();
    const QByteArray getGeometry() const;
    void setGeometry(const QByteArray& geo);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
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
};
