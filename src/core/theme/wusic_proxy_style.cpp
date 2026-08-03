#include "wusic_proxy_style.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionComplex>
#include <QStyleOptionGroupBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>

// ============================================================================
// 构造 / setPalette / standardPalette
// ============================================================================

WusicProxyStyle::WusicProxyStyle(const ThemePalette& palette, const QString& baseKey) :
    QProxyStyle(baseKey), m_palette(palette)
{}

void WusicProxyStyle::setPalette(const ThemePalette& palette)
{
    m_palette = palette;
    if (auto* app = qApp) {
        app->setPalette(m_palette.toQPalette());
    }
}

QPalette WusicProxyStyle::standardPalette() const
{
    return m_palette.toQPalette();
}

// ============================================================================
// pixelMetric —— 尺寸参数
// ============================================================================

int WusicProxyStyle::pixelMetric(PixelMetric m, const QStyleOption* opt, const QWidget* w) const
{
    switch (m) {
    case PM_ScrollBarExtent:
        return m_palette.scrollbarWidth;
    case PM_SliderThickness:
        return m_palette.sliderGrooveH + 8;
    case PM_SliderLength:
        return m_palette.sliderHandleW;
    case PM_TabBarTabHSpace:
        return 12;
    case PM_TabBarTabVSpace:
        return 4;
    case PM_ToolBarIconSize:
        return 20;
    case PM_ToolBarSeparatorExtent:
        return 1;
    case PM_ToolBarFrameWidth:
        return 0;
    case PM_CheckBoxLabelSpacing:
        return 6;
    case PM_RadioButtonLabelSpacing:
        return 6;
    default:
        return QProxyStyle::pixelMetric(m, opt, w);
    }
}

// ============================================================================
// subControlRect
// ============================================================================

QRect WusicProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                                      SubControl sc, const QWidget* w) const
{
    return QProxyStyle::subControlRect(cc, opt, sc, w);
}

// ============================================================================
// styleHint
// ============================================================================

int WusicProxyStyle::styleHint(StyleHint hint, const QStyleOption* opt, const QWidget* w,
                               QStyleHintReturn* ret) const
{
    switch (hint) {
    case SH_EtchDisabledText:
        return 0;
    case SH_DitherDisabledText:
        return 0;
    case SH_Menu_Scrollable:
        return 1;
    case SH_TabBar_Alignment:
        return Qt::AlignLeft;
    default:
        return QProxyStyle::styleHint(hint, opt, w, ret);
    }
}

// ============================================================================
// drawControl —— 控件级绘制
// ============================================================================

void WusicProxyStyle::drawControl(ControlElement element, const QStyleOption* opt, QPainter* p,
                                  const QWidget* w) const
{
    switch (element) {

    // --- 按钮 ---
    case CE_PushButton:
        drawButton(opt, p);
        break;
    case CE_ToolButtonLabel: {
        const auto* tb = qstyleoption_cast<const QStyleOptionToolButton*>(opt);
        if (tb) {
            QStyleOptionToolButton copy = *tb;
            copy.palette.setColor(QPalette::Text, m_palette.text);
            copy.palette.setColor(QPalette::WindowText, m_palette.text);
            copy.palette.setColor(QPalette::ButtonText, m_palette.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 菜单栏 ---
    case CE_MenuBarEmptyArea:
        p->fillRect(opt->rect, m_palette.menuBarBg);
        break;
    case CE_MenuBarItem:
        drawMenuBarItem(opt, p);
        break;

    // --- 下拉菜单 ---
    case CE_MenuItem:
        drawMenuItem(opt, p);
        break;
    case CE_MenuScroller:
    case CE_MenuTearoff:
    case CE_MenuEmptyArea:
        p->fillRect(opt->rect, m_palette.base);
        break;

    // --- 表头 ---
    case CE_Header:
        drawHeader(opt, p);
        break;
    case CE_HeaderLabel: {
        const auto* hdr = qstyleoption_cast<const QStyleOptionHeader*>(opt);
        if (hdr) {
            QStyleOptionHeader copy = *hdr;
            copy.palette.setColor(QPalette::Text, m_palette.text);
            copy.palette.setColor(QPalette::WindowText, m_palette.text);
            copy.palette.setColor(QPalette::ButtonText, m_palette.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 进度条：groove 中一并绘制背景 + 填充，确保填充在上层 ---
    case CE_ProgressBarGroove: {
        // 背景
        p->fillRect(opt->rect, m_palette.progressBarBg);
        // 填充
        const auto* pb = qstyleoption_cast<const QStyleOptionProgressBar*>(opt);
        if (pb && pb->maximum > pb->minimum) {
            const qreal ratio =
                static_cast<qreal>(pb->progress - pb->minimum) / (pb->maximum - pb->minimum);
            QRect fillRect = opt->rect;
            fillRect.setWidth(static_cast<int>(fillRect.width() * ratio));
            if (fillRect.width() > 0 && fillRect.height() > 0)
                fill_round(p, fillRect, m_palette.progressBarFill, m_palette.buttonRadius);
        }
        break;
    }
    case CE_ProgressBarContents:
        break; // 已在 groove 中绘制
    case CE_ProgressBarLabel:
        QProxyStyle::drawControl(element, opt, p, w);
        break;

    // --- 标签页 ---
    case CE_TabBarTab:
        drawTabBarTab(opt, p);
        break;
    case CE_TabBarTabLabel: {
        const auto* tab = qstyleoption_cast<const QStyleOptionTab*>(opt);
        if (tab) {
            QStyleOptionTab copy = *tab;
            const bool sel       = opt->state & QStyle::State_Selected;
            QColor c             = sel ? m_palette.highlight : m_palette.text;
            copy.palette.setColor(QPalette::Text, c);
            copy.palette.setColor(QPalette::WindowText, c);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 复选框 / 单选框文字 ---
    case CE_CheckBoxLabel:
    case CE_RadioButtonLabel: {
        // 注意：原始 opt 是 QStyleOptionButton，不能切片为 QStyleOption
        const auto* src = qstyleoption_cast<const QStyleOptionButton*>(opt);
        if (src) {
            QStyleOptionButton copy = *src;
            copy.palette.setColor(QPalette::Text, m_palette.text);
            copy.palette.setColor(QPalette::WindowText, m_palette.text);
            copy.palette.setColor(QPalette::ButtonText, m_palette.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 分割条 ---
    case CE_Splitter:
        p->fillRect(opt->rect, m_palette.splitterHandle);
        break;

    // --- 滚动条 ---
    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine:
        break;
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage:
        p->fillRect(opt->rect, m_palette.scrollbarBg);
        break;

    default:
        QProxyStyle::drawControl(element, opt, p, w);
        break;
    }
}

// ============================================================================
// drawPrimitive —— 图元级绘制
// ============================================================================

void WusicProxyStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* p,
                                    const QWidget* w) const
{
    switch (pe) {

    // --- 面板背景 ---
    case PE_Widget:
        p->fillRect(opt->rect, m_palette.window);
        break;
    case PE_PanelMenuBar:
        p->fillRect(opt->rect, m_palette.menuBarBg);
        break;
    case PE_PanelToolBar:
        p->fillRect(opt->rect, m_palette.controlBarBg);
        break;
    case PE_PanelMenu: {
        fill_round(p, opt->rect.adjusted(1, 1, -1, -1), m_palette.base, m_palette.menuRadius);
        stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), m_palette.frameBorder,
                    m_palette.menuRadius);
        break;
    }
    case PE_PanelLineEdit:
        p->fillRect(opt->rect, m_palette.base);
        break;
    case PE_PanelStatusBar:
        p->fillRect(opt->rect, m_palette.controlBarBg);
        break;
    case PE_PanelScrollAreaCorner:
        p->fillRect(opt->rect, m_palette.scrollbarBg);
        break;

    // --- 列表/树 ---
    case PE_PanelItemViewItem: {
        if (opt->state & QStyle::State_Selected) {
            p->fillRect(opt->rect, m_palette.itemSelected);
        } else if (opt->state & QStyle::State_MouseOver) {
            p->fillRect(opt->rect, m_palette.itemHover);
        } else {
            QProxyStyle::drawPrimitive(pe, opt, p, w);
        }
        break;
    }
    case PE_IndicatorBranch:
        drawBranchIndicator(opt, p);
        break;

    // --- 边框 ---
    case PE_Frame:
    case PE_FrameDefaultButton:
        stroke_round(p, opt->rect.adjusted(0, 0, -1, -1), m_palette.frameBorder,
                    m_palette.buttonRadius);
        break;
    case PE_FrameGroupBox:
        p->setPen(QPen(m_palette.frameBorder, m_palette.separatorW));
        p->setBrush(Qt::NoBrush);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        break;
    case PE_FrameTabWidget:
        p->fillRect(opt->rect, m_palette.base);
        p->setPen(QPen(m_palette.frameBorder, 1));
        p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
        break;
    case PE_FrameWindow:
        p->fillRect(opt->rect, m_palette.window);
        break;

    // --- 指示器 ---
    case PE_IndicatorCheckBox:
        drawCheckIndicator(opt, p);
        break;
    case PE_IndicatorRadioButton:
        drawRadioIndicator(opt, p);
        break;
    case PE_IndicatorMenuCheckMark: {
        const int x = opt->rect.center().x();
        const int y = opt->rect.center().y();
        QPen pen(m_palette.highlight, 2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p->setPen(pen);
        p->setBrush(Qt::NoBrush);
        QPainterPath check;
        check.moveTo(x - 3, y);
        check.lineTo(x - 1, y + 3);
        check.lineTo(x + 4, y - 3);
        p->drawPath(check);
        break;
    }
    case PE_IndicatorHeaderArrow: {
        QStyleOption copy = *opt;
        copy.palette.setColor(QPalette::ButtonText, m_palette.text);
        QProxyStyle::drawPrimitive(pe, &copy, p, w);
        break;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        QStyleOption copy = *opt;
        copy.palette.setColor(QPalette::ButtonText, m_palette.text);
        QProxyStyle::drawPrimitive(pe, &copy, p, w);
        break;
    }
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        const int cx = opt->rect.center().x(), cy = opt->rect.center().y();
        const bool up = (pe == PE_IndicatorSpinUp);
        QPainterPath tri;
        if (up) {
            tri.moveTo(cx - 3, cy + 1);
            tri.lineTo(cx, cy - 2);
            tri.lineTo(cx + 3, cy + 1);
        } else {
            tri.moveTo(cx - 3, cy - 1);
            tri.lineTo(cx, cy + 2);
            tri.lineTo(cx + 3, cy - 1);
        }
        tri.closeSubpath();
        p->setPen(Qt::NoPen);
        p->setBrush(m_palette.text);
        p->drawPath(tri);
        break;
    }
    case PE_IndicatorTabClose:
        QProxyStyle::drawPrimitive(pe, opt, p, w);
        break;

    // --- 工具栏 ---
    case PE_IndicatorToolBarSeparator:
        p->setPen(QPen(m_palette.frameBorder, 1));
        p->drawLine(opt->rect.center().x(), opt->rect.top() + 4, opt->rect.center().x(),
                    opt->rect.bottom() - 4);
        break;
    case PE_IndicatorToolBarHandle:
        p->fillRect(opt->rect, m_palette.controlBarBg);
        break;

    default:
        QProxyStyle::drawPrimitive(pe, opt, p, w);
        break;
    }
}

// ============================================================================
// drawComplexControl —— 复合控件绘制
// ============================================================================

void WusicProxyStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt,
                                         QPainter* p, const QWidget* w) const
{
    switch (cc) {
    case CC_Slider:
        drawSlider(opt, p);
        break;
    case CC_ScrollBar:
        drawScrollBar(cc, opt, p, w);
        break;
    case CC_ComboBox:
        drawComboBox(opt, p);
        break;
    case CC_SpinBox:
        drawSpinBox(opt, p, w);
        break;
    case CC_ToolButton:
        drawToolButton(opt, p);
        break;
    default:
        QProxyStyle::drawComplexControl(cc, opt, p, w);
        break;
    }
}

// ============================================================================
// polish / unpolish
// ============================================================================

void WusicProxyStyle::polish(QWidget* w)
{
    if (auto* tree = qobject_cast<QTreeView*>(w)) {
        tree->setAlternatingRowColors(true);
    }
    if (auto* le = qobject_cast<QLineEdit*>(w))
        le->setFrame(false);
    if (auto* cb = qobject_cast<QComboBox*>(w))
        cb->setFrame(false);
    if (auto* sb = qobject_cast<QSpinBox*>(w))
        sb->setFrame(false);
    if (auto* ds = qobject_cast<QDoubleSpinBox*>(w))
        ds->setFrame(false);
    if (auto* sc = qobject_cast<QScrollBar*>(w))
        sc->setAttribute(Qt::WA_Hover, true);
    if (auto* tb = qobject_cast<QToolButton*>(w))
        tb->setAutoRaise(true);
    QProxyStyle::polish(w);
}

void WusicProxyStyle::unpolish(QWidget* w)
{
    QProxyStyle::unpolish(w);
}

// ============================================================================
// 通用辅助
// ============================================================================

void WusicProxyStyle::fill_round(QPainter* p, const QRect& r, const QColor& bg, int /*radius*/) const
{
    p->fillRect(r, bg);
}

void WusicProxyStyle::stroke_round(QPainter* p, const QRect& r, const QColor& border, int /*radius*/,
                                  int w) const
{
    p->setPen(QPen(border, w));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(w / 2, w / 2, -w / 2, -w / 2));
}

// ============================================================================
// drawButton
// ============================================================================

void WusicProxyStyle::drawButton(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r       = opt->rect.adjusted(1, 1, -1, -1);
    const bool sunken   = opt->state & QStyle::State_Sunken;
    const bool hovered  = opt->state & QStyle::State_MouseOver;
    const bool enabled  = opt->state & QStyle::State_Enabled;
    const bool hasFocus = opt->state & QStyle::State_HasFocus;

    QColor bg           = m_palette.button;
    // 如果控件设置了 autoFillBackground，优先用其自身的 Button 色（如颜色预览按钮）
    if (const auto* obj = opt->styleObject) {
        if (auto* w = qobject_cast<const QWidget*>(obj)) {
            if (w->autoFillBackground())
                bg = opt->palette.color(QPalette::Button);
        }
    }
    if (!enabled)
        bg = bg.darker(150);
    else if (sunken)
        bg = m_palette.highlight;
    else if (hovered)
        bg = m_palette.itemHover;

    fill_round(p, r, bg, m_palette.buttonRadius);

    const auto* btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
    if (!btn) {
        p->restore();
        return;
    }

    // 图标 + 文字布局
    const bool hasIcon = !btn->icon.isNull();
    const bool hasText = !btn->text.isEmpty();
    QRect contentRect  = r;

    if (hasIcon && hasText) {
        // 图标居左，文字居中
        const int iconSize = qMin(r.height() - 6, btn->iconSize.width());
        QRect iconRect(r.left() + 6, r.center().y() - iconSize / 2, iconSize, iconSize);
        btn->icon.paint(p, iconRect);
        contentRect.setLeft(iconRect.right() + 4);
    } else if (hasIcon) {
        // 仅图标，居中
        const int iconSize = qMin(r.height() - 6, btn->iconSize.width());
        QRect iconRect(r.center().x() - iconSize / 2, r.center().y() - iconSize / 2, iconSize,
                       iconSize);
        btn->icon.paint(p, iconRect);
    }

    // 文字
    if (hasText) {
        QPalette::ColorRole textRole = sunken ? QPalette::HighlightedText : QPalette::ButtonText;
        drawItemText(p, contentRect, Qt::AlignCenter | Qt::TextShowMnemonic, opt->palette, enabled,
                     btn->text, textRole);
    }

    // 焦点虚线框
    if (hasFocus && enabled) {
        QStyleOptionFocusRect frOpt;
        frOpt.rect    = r.adjusted(2, 2, -2, -2);
        frOpt.palette = opt->palette;
        frOpt.state   = opt->state;
        QProxyStyle::drawPrimitive(PE_FrameFocusRect, &frOpt, p, nullptr);
    }

    p->restore();
}

// ============================================================================
// drawToolButton
// ============================================================================

void WusicProxyStyle::drawToolButton(const QStyleOptionComplex* opt, QPainter* p) const
{
    const bool hovered = opt->state & QStyle::State_MouseOver;
    const bool sunken  = opt->state & QStyle::State_Sunken;

    if (hovered || sunken) {
        QColor bg = sunken ? m_palette.highlight : m_palette.itemHover;
        fill_round(p, opt->rect.adjusted(2, 2, -2, -2), bg, m_palette.buttonRadius);
    }
}

// ============================================================================
// drawSlider
// ============================================================================

void WusicProxyStyle::drawSlider(const QStyleOption* opt, QPainter* p) const
{
    const auto* sl = qstyleoption_cast<const QStyleOptionSlider*>(opt);
    if (!sl) {
        const auto* co = qstyleoption_cast<const QStyleOptionComplex*>(opt);
        QProxyStyle::drawComplexControl(CC_Slider, co, p, nullptr);
        return;
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const bool horiz = sl->orientation == Qt::Horizontal;
    const QRect gr   = QProxyStyle::subControlRect(CC_Slider, sl, SC_SliderGroove, nullptr);
    const QRect hr   = QProxyStyle::subControlRect(CC_Slider, sl, SC_SliderHandle, nullptr);

    QRectF groove;
    if (horiz) {
        groove = QRectF(gr.x(), gr.center().y() - m_palette.sliderGrooveH / 2.0, gr.width(),
                        m_palette.sliderGrooveH);
    } else {
        groove = QRectF(gr.center().x() - m_palette.sliderGrooveH / 2.0, gr.y(),
                        m_palette.sliderGrooveH, gr.height());
    }

    // 已过部分：从滑槽起点到滑块中心
    QRectF filled = groove;
    if (horiz) {
        filled.setWidth(hr.center().x() - groove.x());
    } else {
        const qreal filledBottom = hr.center().y();
        filled.setTop(filledBottom);
    }

    p->setPen(Qt::NoPen);

    // 已过部分
    p->fillRect(filled, m_palette.progressBarFill);

    // 未过部分
    if (horiz) {
        QRectF unfilled(filled.right(), groove.y(), groove.right() - filled.right(),
                        groove.height());
        p->fillRect(unfilled, m_palette.progressBarBg);
    } else {
        QRectF unfilled(groove.x(), groove.y(), groove.width(), filled.y() - groove.y());
        p->fillRect(unfilled, m_palette.progressBarBg);
    }

    const bool hovered = opt->state & QStyle::State_MouseOver;
    QColor hc          = hovered ? m_palette.highlight.lighter(120) : m_palette.highlight;
    p->setBrush(hc);

    // 圆形手柄可能超出控件裁剪区域，扩展现有裁剪区以包含手柄
    if (p->hasClipping()) {
        QRegion clip = p->clipRegion();
        clip += hr.adjusted(-2, -2, 2, 2); // 手柄周围留 2px 余量
        p->setClipRegion(clip);
    }
    p->drawEllipse(hr);

    p->restore();
}

// ============================================================================
// drawScrollBar —— 完全自定义
// ============================================================================

void WusicProxyStyle::drawScrollBar(ComplexControl /*cc*/, const QStyleOptionComplex* opt,
                                    QPainter* p, const QWidget* w) const
{
    const auto* sb = qstyleoption_cast<const QStyleOptionSlider*>(opt);
    if (!sb)
        return;

    const bool horiz = sb->orientation == Qt::Horizontal;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // 1) 整条滑槽背景
    p->fillRect(opt->rect, m_palette.scrollbarBg);

    // 2) 箭头按钮（绘制小三角，隐藏 Fusion 默认按钮）
    auto drawArrowBtn = [&](SubControl sc, bool isUpOrLeft) {
        QRect r = QProxyStyle::subControlRect(CC_ScrollBar, sb, sc, w);
        if (r.isNull())
            return;
        // 按钮背景略亮于滑槽
        p->fillRect(r, m_palette.button);
        // 三角形
        int cx = r.center().x(), cy = r.center().y();
        p->setPen(m_palette.text);
        p->setBrush(m_palette.text);
        QPainterPath tri;
        if (horiz) {
            if (isUpOrLeft) {
                tri.moveTo(cx - 3, cy);
                tri.lineTo(cx + 3, cy - 3);
                tri.lineTo(cx + 3, cy + 3);
            } else {
                tri.moveTo(cx + 3, cy);
                tri.lineTo(cx - 3, cy - 3);
                tri.lineTo(cx - 3, cy + 3);
            }
        } else {
            if (isUpOrLeft) {
                tri.moveTo(cx, cy - 3);
                tri.lineTo(cx - 3, cy + 3);
                tri.lineTo(cx + 3, cy + 3);
            } else {
                tri.moveTo(cx, cy + 3);
                tri.lineTo(cx - 3, cy - 3);
                tri.lineTo(cx + 3, cy - 3);
            }
        }
        tri.closeSubpath();
        p->drawPath(tri);
    };
    drawArrowBtn(SC_ScrollBarSubLine, true);
    drawArrowBtn(SC_ScrollBarAddLine, false);

    // 3) 滑块
    QRect sr = QProxyStyle::subControlRect(CC_ScrollBar, sb, SC_ScrollBarSlider, w);
    if (!sr.isNull()) {
        const bool hovered = opt->state & QStyle::State_MouseOver;
        QColor handleColor =
            hovered ? m_palette.scrollbarHandle.lighter(130) : m_palette.scrollbarHandle;
        if (horiz)
            sr.adjust(0, 2, 0, -2);
        else
            sr.adjust(2, 0, -2, 0);
        p->fillRect(sr, handleColor);
    }

    p->restore();
}

// ============================================================================
// drawMenuItem
// ============================================================================

void WusicProxyStyle::drawMenuItem(const QStyleOption* opt, QPainter* p) const
{
    const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
    if (!mi || mi->menuItemType == QStyleOptionMenuItem::Separator) {
        if (mi) {
            p->setPen(QPen(m_palette.frameBorder, 1));
            p->drawLine(opt->rect.left() + 8, opt->rect.center().y(), opt->rect.right() - 8,
                        opt->rect.center().y());
        }
        return;
    }

    const bool sel = opt->state & QStyle::State_Selected;
    const bool ena = opt->state & QStyle::State_Enabled;

    if (sel && ena) {
        fill_round(p, opt->rect.adjusted(2, 1, 2, -1), m_palette.highlight, m_palette.menuRadius);
    }

    QStyleOptionMenuItem copy = *mi;
    copy.palette.setColor(QPalette::Text, sel ? m_palette.highlightedText : m_palette.text);
    copy.palette.setColor(QPalette::HighlightedText, m_palette.highlightedText);

    if (!mi->icon.isNull()) {
        QRect ir = opt->rect;
        ir.setWidth(ir.height());
        mi->icon.paint(p, ir.adjusted(4, 4, -4, -4));
    }

    QRect tr = opt->rect.adjusted(mi->maxIconWidth + 8, 0, -12, 0);
    drawItemText(p, tr, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, copy.palette, ena,
                 mi->text, sel ? QPalette::HighlightedText : QPalette::Text);
}

// ============================================================================
// drawMenuBarItem
// ============================================================================

void WusicProxyStyle::drawMenuBarItem(const QStyleOption* opt, QPainter* p) const
{
    const bool sel = opt->state & QStyle::State_Selected;
    const bool ena = opt->state & QStyle::State_Enabled;

    if (sel && ena) {
        fill_round(p, opt->rect.adjusted(2, 2, -2, -2), m_palette.itemHover, m_palette.buttonRadius);
    }

    const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
    if (mi) {
        QPalette::ColorRole role = (sel && ena) ? QPalette::HighlightedText : QPalette::Text;
        drawItemText(p, opt->rect, Qt::AlignCenter | Qt::TextShowMnemonic, opt->palette, ena,
                     mi->text, role);
    }
}

// ============================================================================
// drawHeader
// ============================================================================

void WusicProxyStyle::drawHeader(const QStyleOption* opt, QPainter* p) const
{
    const auto* hdr    = qstyleoption_cast<const QStyleOptionHeader*>(opt);
    const bool hovered = opt->state & QStyle::State_MouseOver;
    const bool sunken  = opt->state & QStyle::State_Sunken;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // 背景
    QColor bg = m_palette.button;
    if (sunken)
        bg = m_palette.highlight;
    else if (hovered)
        bg = m_palette.itemHover;
    fill_round(p, opt->rect, bg, m_palette.buttonRadius);

    // 底部边框
    p->setPen(QPen(m_palette.frameBorder, 1));
    p->drawLine(opt->rect.bottomLeft(), opt->rect.bottomRight());

    // 列间分隔线（右边缘），用 splitterHandle 颜色与底部边框区分
    p->setPen(QPen(m_palette.splitterHandle, 1));
    p->drawLine(opt->rect.topRight() + QPoint(0, 4), opt->rect.bottomRight() + QPoint(0, -4));

    // 文字 + 排序箭头
    if (hdr) {
        QStyleOptionHeader copy = *hdr;
        copy.palette.setColor(QPalette::Text, m_palette.text);
        copy.palette.setColor(QPalette::WindowText, m_palette.text);
        copy.palette.setColor(QPalette::ButtonText, m_palette.text);

        // 文字
        if (!hdr->text.isEmpty()) {
            QRect textRect = opt->rect.adjusted(4, 0, -4, 0);
            // 有排序箭头时右侧留空
            if (hdr->sortIndicator != QStyleOptionHeader::None) {
                textRect.setRight(textRect.right() - 16);
            }
            drawItemText(p, textRect, hdr->textAlignment | Qt::AlignVCenter | Qt::TextShowMnemonic,
                         copy.palette, true, hdr->text, QPalette::Text);
        }

        // 排序箭头：紧贴右边缘，QPainter 直接绘制
        if (hdr->sortIndicator != QStyleOptionHeader::None) {
            const int cx  = opt->rect.right() - 8; // 箭头中心 X，距右边缘 8px
            const int cy  = opt->rect.center().y();
            const bool up = (hdr->sortIndicator == QStyleOptionHeader::SortUp);

            p->setPen(QPen(m_palette.text, 1.5));
            p->setBrush(Qt::NoBrush);
            QPainterPath arrow;
            if (up) {
                arrow.moveTo(cx - 3, cy + 1);
                arrow.lineTo(cx, cy - 2);
                arrow.lineTo(cx + 3, cy + 1);
            } else {
                arrow.moveTo(cx - 3, cy - 1);
                arrow.lineTo(cx, cy + 2);
                arrow.lineTo(cx + 3, cy - 1);
            }
            p->drawPath(arrow);
        }
    }

    p->restore();
}

// ============================================================================
// drawComboBox
// ============================================================================

void WusicProxyStyle::drawComboBox(const QStyleOptionComplex* opt, QPainter* p) const
{
    const auto* cb = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
    if (!cb) {
        QProxyStyle::drawComplexControl(CC_ComboBox, opt, p, nullptr);
        return;
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const bool ena     = opt->state & QStyle::State_Enabled;
    const bool hovered = opt->state & QStyle::State_MouseOver;

    QColor bg          = ena ? m_palette.base : m_palette.base.darker(120);
    QColor bd          = hovered ? m_palette.highlight : m_palette.frameBorder;

    fill_round(p, opt->rect.adjusted(1, 1, -1, -1), bg, m_palette.buttonRadius);
    stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), bd, m_palette.buttonRadius);

    QRect arrow = QProxyStyle::subControlRect(CC_ComboBox, cb, SC_ComboBoxArrow, nullptr);
    if (!arrow.isNull()) {
        QPainterPath ap;
        int cx = arrow.center().x(), cy = arrow.center().y();
        ap.moveTo(cx - 4, cy - 2);
        ap.lineTo(cx, cy + 3);
        ap.lineTo(cx + 4, cy - 2);
        ap.closeSubpath();
        p->setBrush(m_palette.text);
        p->setPen(Qt::NoPen);
        p->drawPath(ap);
    }

    if (!cb->currentText.isEmpty()) {
        QRect tr = QProxyStyle::subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, nullptr);
        drawItemText(p, tr, Qt::AlignLeft | Qt::AlignVCenter, opt->palette, ena, cb->currentText,
                     QPalette::Text);
    }

    p->restore();
}

// ============================================================================
// drawSpinBox
// ============================================================================

void WusicProxyStyle::drawSpinBox(const QStyleOptionComplex* opt, QPainter* p,
                                  const QWidget* w) const
{
    const auto* sb = qstyleoption_cast<const QStyleOptionSpinBox*>(opt);
    if (!sb) {
        QProxyStyle::drawComplexControl(CC_SpinBox, opt, p, w);
        return;
    }

    p->save();

    // 1) 背景 + 边框
    fill_round(p, opt->rect.adjusted(1, 1, -1, -1), m_palette.base, m_palette.buttonRadius);
    stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), m_palette.frameBorder, m_palette.buttonRadius);

    // 2) 上下按钮背景
    QRect upRect   = QProxyStyle::subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, w);
    QRect downRect = QProxyStyle::subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, w);
    if (!upRect.isNull())
        p->fillRect(upRect.adjusted(1, 1, 0, 0), m_palette.button);
    if (!downRect.isNull())
        p->fillRect(downRect.adjusted(1, 0, 0, -1), m_palette.button);

    // 3) 箭头（在按钮背景之上）
    if (!upRect.isNull()) {
        QStyleOption upOpt;
        upOpt.rect = upRect;
        drawPrimitive(PE_IndicatorSpinUp, &upOpt, p, w);
    }
    if (!downRect.isNull()) {
        QStyleOption downOpt;
        downOpt.rect = downRect;
        drawPrimitive(PE_IndicatorSpinDown, &downOpt, p, w);
    }

    p->restore();
}

// ============================================================================
// drawTabBarTab
// ============================================================================

void WusicProxyStyle::drawTabBarTab(const QStyleOption* opt, QPainter* p) const
{
    const bool sel     = opt->state & QStyle::State_Selected;
    const bool hovered = opt->state & QStyle::State_MouseOver;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    QColor bg = sel ? m_palette.base : (hovered ? m_palette.itemHover : m_palette.window);
    fill_round(p, opt->rect.adjusted(2, 2, -2, opt->rect.height() / 2), bg, m_palette.buttonRadius);

    if (sel) {
        p->fillRect(QRect(opt->rect.left() + 4, opt->rect.bottom() - 2, opt->rect.width() - 8, 2),
                    m_palette.highlight);
    }

    p->restore();
}

// ============================================================================
// drawCheckIndicator
// ============================================================================

void WusicProxyStyle::drawCheckIndicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r      = opt->rect.adjusted(2, 2, -2, -2);
    const bool checked = opt->state & QStyle::State_On;
    const bool enabled = opt->state & QStyle::State_Enabled;

    QColor bg          = checked ? m_palette.highlight : m_palette.base;
    QColor bd          = enabled ? m_palette.frameBorder : m_palette.frameBorder.darker(150);

    fill_round(p, r, bg, m_palette.buttonRadius);
    stroke_round(p, r, bd, m_palette.buttonRadius);

    if (checked) {
        QPen pen(m_palette.highlightedText, 2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p->setPen(pen);
        QPainterPath check;
        int cx = r.center().x(), cy = r.center().y();
        check.moveTo(cx - 3, cy);
        check.lineTo(cx - 1, cy + 3);
        check.lineTo(cx + 4, cy - 3);
        p->drawPath(check);
    }

    p->restore();
}

// ============================================================================
// drawRadioIndicator
// ============================================================================

void WusicProxyStyle::drawRadioIndicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r      = opt->rect.adjusted(2, 2, -2, -2);
    const bool checked = opt->state & QStyle::State_On;
    const bool enabled = opt->state & QStyle::State_Enabled;

    QColor bd          = enabled ? (checked ? m_palette.highlight : m_palette.frameBorder)
                                 : m_palette.frameBorder.darker(150);

    p->setPen(QPen(bd, 2));
    p->setBrush(m_palette.base);
    p->drawEllipse(r);

    if (checked) {
        p->setPen(Qt::NoPen);
        p->setBrush(m_palette.highlight);
        p->drawEllipse(r.adjusted(3, 3, -3, -3));
    }

    p->restore();
}

// ============================================================================
// drawBranchIndicator
// ============================================================================

void WusicProxyStyle::drawBranchIndicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const int cx = opt->rect.center().x();
    const int cy = opt->rect.center().y();

    p->setPen(QPen(m_palette.text, 1.5));
    p->setBrush(Qt::NoBrush);

    if (opt->state & QStyle::State_Children) {
        QPainterPath arrow;
        if (opt->state & QStyle::State_Open) {
            arrow.moveTo(cx - 4, cy - 2);
            arrow.lineTo(cx, cy + 3);
            arrow.lineTo(cx + 4, cy - 2);
        } else {
            arrow.moveTo(cx - 2, cy - 4);
            arrow.lineTo(cx + 3, cy);
            arrow.lineTo(cx - 2, cy + 4);
        }
        p->drawPath(arrow);
    }

    p->restore();
}
