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

WusicProxyStyle::WusicProxyStyle(ThemePalette palette, const QString& base_key) :
    QProxyStyle(base_key), palette_(std::move(palette))
{}

void WusicProxyStyle::set_palette(const ThemePalette& palette)
{
    palette_ = palette;
    if (qApp) {
        QApplication::setPalette(palette_.to_qpalette());
    }
}

QPalette WusicProxyStyle::standardPalette() const
{
    return palette_.to_qpalette();
}

// ============================================================================
// pixelMetric —— 尺寸参数
// ============================================================================

int WusicProxyStyle::pixelMetric(PixelMetric m, const QStyleOption* opt, const QWidget* w) const
{
    switch (m) {
    case PM_ScrollBarExtent:
        return palette_.scroll_bar_width;
    case PM_SliderThickness:
        return palette_.slider_groove_h + 8;
    case PM_SliderLength:
        return palette_.slider_handle_w;
    case PM_TabBarTabHSpace:
        return 12;
    case PM_TabBarTabVSpace: {
        // 标签总高度 = 字体高度 + 垂直留白，由目标高度反推留白
        const int font_h = (w && qobject_cast<const QTabBar*>(w)) ? w->fontMetrics().height() : 16;
        return qMax(palette_.tab_bar_height - font_h, 2);
    }
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
        draw_button(opt, p);
        break;
    case CE_ToolButtonLabel: {
        const auto* tb = qstyleoption_cast<const QStyleOptionToolButton*>(opt);
        if (tb) {
            QStyleOptionToolButton copy = *tb;
            copy.palette.setColor(QPalette::Text, palette_.text);
            copy.palette.setColor(QPalette::WindowText, palette_.text);
            copy.palette.setColor(QPalette::ButtonText, palette_.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 菜单栏 ---
    case CE_MenuBarEmptyArea:
        p->fillRect(opt->rect, palette_.menu_bar_bg);
        break;
    case CE_MenuBarItem:
        draw_menu_bar_item(opt, p);
        break;

    // --- 下拉菜单 ---
    case CE_MenuItem:
        draw_menu_item(opt, p, w);
        break;
    case CE_MenuScroller:
    case CE_MenuTearoff:
    case CE_MenuEmptyArea:
        p->fillRect(opt->rect, palette_.base);
        break;

    // --- 表头 ---
    case CE_Header:
        draw_header(opt, p);
        break;
    case CE_HeaderLabel: {
        const auto* hdr = qstyleoption_cast<const QStyleOptionHeader*>(opt);
        if (hdr) {
            QStyleOptionHeader copy = *hdr;
            copy.palette.setColor(QPalette::Text, palette_.text);
            copy.palette.setColor(QPalette::WindowText, palette_.text);
            copy.palette.setColor(QPalette::ButtonText, palette_.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 进度条：groove 中一并绘制背景 + 填充，确保填充在上层 ---
    case CE_ProgressBarGroove: {
        // 背景
        p->fillRect(opt->rect, palette_.progress_bar_bg);
        // 填充
        const auto* pb = qstyleoption_cast<const QStyleOptionProgressBar*>(opt);
        if (pb && pb->maximum > pb->minimum) {
            const qreal ratio =
                static_cast<qreal>(pb->progress - pb->minimum) / (pb->maximum - pb->minimum);
            QRect fill_rect = opt->rect;
            fill_rect.setWidth(static_cast<int>(fill_rect.width() * ratio));
            if (fill_rect.width() > 0 && fill_rect.height() > 0) {
                fill_round(p, fill_rect, palette_.progress_bar_fill, palette_.button_radius);
            }
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
        draw_tab_bar_bab(opt, p, w);
        break;
    case CE_TabBarTabLabel: {
        const auto* tab = qstyleoption_cast<const QStyleOptionTab*>(opt);
        if (tab) {
            QStyleOptionTab copy = *tab;
            const bool sel       = opt->state & QStyle::State_Selected;
            QColor c             = sel ? palette_.highlight : palette_.text;
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
            copy.palette.setColor(QPalette::Text, palette_.text);
            copy.palette.setColor(QPalette::WindowText, palette_.text);
            copy.palette.setColor(QPalette::ButtonText, palette_.text);
            QProxyStyle::drawControl(element, &copy, p, w);
        } else {
            QProxyStyle::drawControl(element, opt, p, w);
        }
        break;
    }

    // --- 分割条 ---
    case CE_Splitter:
        p->fillRect(opt->rect, palette_.splitter_handle);
        break;

    // --- 滚动条 ---
    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine:
        break;
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage:
        p->fillRect(opt->rect, palette_.scroll_bar_bg);
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
        p->fillRect(opt->rect, palette_.window);
        break;
    case PE_PanelMenuBar:
        p->fillRect(opt->rect, palette_.menu_bar_bg);
        break;
    case PE_PanelToolBar:
        p->fillRect(opt->rect, palette_.control_bar_bg);
        break;
    case PE_PanelMenu: {
        fill_round(p, opt->rect.adjusted(1, 1, -1, -1), palette_.base, palette_.menu_radius);
        stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), palette_.frame_border,
                     palette_.menu_radius);
        break;
    }
    case PE_PanelLineEdit:
        p->fillRect(opt->rect, palette_.base);
        break;
    case PE_PanelStatusBar:
        p->fillRect(opt->rect, palette_.control_bar_bg);
        break;
    case PE_PanelScrollAreaCorner:
        p->fillRect(opt->rect, palette_.scroll_bar_bg);
        break;

    // --- 列表/树 ---
    case PE_PanelItemViewItem: {
        if (opt->state & QStyle::State_Selected) {
            p->fillRect(opt->rect, palette_.item_selected);
        } else if (opt->state & QStyle::State_MouseOver) {
            p->fillRect(opt->rect, palette_.item_hover);
        } else {
            QProxyStyle::drawPrimitive(pe, opt, p, w);
        }
        break;
    }
    case PE_IndicatorBranch:
        draw_branch_indicator(opt, p);
        break;

    // --- 边框 ---
    case PE_Frame:
    case PE_FrameDefaultButton:
        stroke_round(p, opt->rect.adjusted(0, 0, -1, -1), palette_.frame_border,
                     palette_.button_radius);
        break;
    case PE_FrameGroupBox:
        p->setPen(QPen(palette_.frame_border, palette_.separator_w));
        p->setBrush(Qt::NoBrush);
        p->drawRect(opt->rect.adjusted(0, 0, -1, -1));
        break;
    case PE_FrameTabWidget:
        p->fillRect(opt->rect, palette_.base);
        p->setPen(QPen(palette_.frame_border, 1));
        p->drawLine(opt->rect.topLeft(), opt->rect.topRight());
        break;
    case PE_FrameWindow:
        p->fillRect(opt->rect, palette_.window);
        break;

    // --- 指示器 ---
    case PE_IndicatorCheckBox:
        draw_check_indicator(opt, p);
        break;
    case PE_IndicatorRadioButton:
        draw_radio_indicator(opt, p);
        break;
    case PE_IndicatorMenuCheckMark: {
        const int x = opt->rect.center().x();
        const int y = opt->rect.center().y();
        QPen pen(palette_.highlight, 2);
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
        copy.palette.setColor(QPalette::ButtonText, palette_.text);
        QProxyStyle::drawPrimitive(pe, &copy, p, w);
        break;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        QStyleOption copy = *opt;
        copy.palette.setColor(QPalette::ButtonText, palette_.text);
        QProxyStyle::drawPrimitive(pe, &copy, p, w);
        break;
    }
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        const int cx  = opt->rect.center().x();
        const int cy  = opt->rect.center().y();
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
        p->setBrush(palette_.text);
        p->drawPath(tri);
        break;
    }
    case PE_IndicatorTabClose:
        QProxyStyle::drawPrimitive(pe, opt, p, w);
        break;

    // --- 工具栏 ---
    case PE_IndicatorToolBarSeparator:
        p->setPen(QPen(palette_.frame_border, 1));
        p->drawLine(opt->rect.center().x(), opt->rect.top() + 4, opt->rect.center().x(),
                    opt->rect.bottom() - 4);
        break;
    case PE_IndicatorToolBarHandle:
        p->fillRect(opt->rect, palette_.control_bar_bg);
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
        draw_slider(opt, p);
        break;
    case CC_ScrollBar:
        draw_scroll_bar(cc, opt, p, w);
        break;
    case CC_ComboBox:
        draw_combo_box(opt, p);
        break;
    case CC_SpinBox:
        draw_spin_box(opt, p, w);
        break;
    case CC_ToolButton:
        draw_tool_button(opt, p);
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
    if (auto* le = qobject_cast<QLineEdit*>(w)) {
        le->setFrame(false);
    }
    if (auto* cb = qobject_cast<QComboBox*>(w)) {
        cb->setFrame(false);
    }
    if (auto* sb = qobject_cast<QSpinBox*>(w)) {
        sb->setFrame(false);
    }
    if (auto* ds = qobject_cast<QDoubleSpinBox*>(w)) {
        ds->setFrame(false);
    }
    if (auto* sc = qobject_cast<QScrollBar*>(w)) {
        sc->setAttribute(Qt::WA_Hover, true);
    }
    if (auto* tb = qobject_cast<QToolButton*>(w)) {
        tb->setAutoRaise(true);
    }
    QProxyStyle::polish(w);
}

void WusicProxyStyle::unpolish(QWidget* w)
{
    QProxyStyle::unpolish(w);
}

// ============================================================================
// 通用辅助
// ============================================================================

void WusicProxyStyle::fill_round(QPainter* p, const QRect& r, const QColor& bg, int /*radius*/)
{
    p->fillRect(r, bg);
}

void WusicProxyStyle::stroke_round(QPainter* p, const QRect& r, const QColor& border,
                                   int /*radius*/, int w)
{
    p->setPen(QPen(border, w));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(w / 2, w / 2, -w / 2, -w / 2));
}

// ============================================================================
// drawButton
// ============================================================================

void WusicProxyStyle::draw_button(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r        = opt->rect.adjusted(1, 1, -1, -1);
    const bool sunken    = opt->state & QStyle::State_Sunken;
    const bool hovered   = opt->state & QStyle::State_MouseOver;
    const bool enabled   = opt->state & QStyle::State_Enabled;
    const bool has_focus = opt->state & QStyle::State_HasFocus;

    QColor bg            = palette_.button;
    // 如果控件设置了 autoFillBackground，优先用其自身的 Button 色（如颜色预览按钮）
    if (const auto* obj = opt->styleObject) {
        if (const auto* w = qobject_cast<const QWidget*>(obj)) {
            if (w->autoFillBackground()) {
                bg = opt->palette.color(QPalette::Button);
            }
        }
    }
    if (!enabled) {
        bg = bg.darker(150);
    } else if (sunken) {
        bg = palette_.highlight;
    } else if (hovered) {
        bg = palette_.item_hover;
    }

    fill_round(p, r, bg, palette_.button_radius);

    const auto* btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
    if (!btn) {
        p->restore();
        return;
    }

    // 图标 + 文字布局
    const bool has_icon = !btn->icon.isNull();
    const bool has_text = !btn->text.isEmpty();
    QRect content_rect  = r;

    if (has_icon && has_text) {
        // 图标居左，文字居中
        const int icon_size = qMin(r.height() - 6, btn->iconSize.width());
        QRect icon_rect(r.left() + 6, r.center().y() - (icon_size / 2), icon_size, icon_size);
        btn->icon.paint(p, icon_rect);
        content_rect.setLeft(icon_rect.right() + 4);
    } else if (has_icon) {
        // 仅图标，居中
        const int icon_size = qMin(r.height() - 6, btn->iconSize.width());
        QRect icon_rect(r.center().x() - (icon_size / 2), r.center().y() - (icon_size / 2),
                        icon_size, icon_size);
        btn->icon.paint(p, icon_rect);
    }

    // 文字
    if (has_text) {
        QPalette::ColorRole text_role = sunken ? QPalette::HighlightedText : QPalette::ButtonText;
        drawItemText(p, content_rect, Qt::AlignCenter | Qt::TextShowMnemonic, opt->palette, enabled,
                     btn->text, text_role);
    }

    // 焦点虚线框
    if (has_focus && enabled) {
        QStyleOptionFocusRect fr_opt;
        fr_opt.rect    = r.adjusted(2, 2, -2, -2);
        fr_opt.palette = opt->palette;
        fr_opt.state   = opt->state;
        QProxyStyle::drawPrimitive(PE_FrameFocusRect, &fr_opt, p, nullptr);
    }

    p->restore();
}

// ============================================================================
// drawToolButton
// ============================================================================

void WusicProxyStyle::draw_tool_button(const QStyleOptionComplex* opt, QPainter* p) const
{
    const bool hovered = opt->state & QStyle::State_MouseOver;
    const bool sunken  = opt->state & QStyle::State_Sunken;

    if (hovered || sunken) {
        QColor bg = sunken ? palette_.highlight : palette_.item_hover;
        fill_round(p, opt->rect.adjusted(2, 2, -2, -2), bg, palette_.button_radius);
    }
}

// ============================================================================
// drawSlider
// ============================================================================

void WusicProxyStyle::draw_slider(const QStyleOption* opt, QPainter* p) const
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
        groove = QRectF(gr.x(), gr.center().y() - (palette_.slider_groove_h / 2.0), gr.width(),
                        palette_.slider_groove_h);
    } else {
        groove = QRectF(gr.center().x() - (palette_.slider_groove_h / 2.0), gr.y(),
                        palette_.slider_groove_h, gr.height());
    }

    // 已过部分：从滑槽起点到滑块中心
    QRectF filled = groove;
    if (horiz) {
        filled.setWidth(hr.center().x() - groove.x());
    } else {
        const qreal filled_bottom = hr.center().y();
        filled.setTop(filled_bottom);
    }

    p->setPen(Qt::NoPen);

    // 已过部分
    p->fillRect(filled, palette_.progress_bar_fill);

    // 未过部分
    if (horiz) {
        QRectF unfilled(filled.right(), groove.y(), groove.right() - filled.right(),
                        groove.height());
        p->fillRect(unfilled, palette_.progress_bar_bg);
    } else {
        QRectF unfilled(groove.x(), groove.y(), groove.width(), filled.y() - groove.y());
        p->fillRect(unfilled, palette_.progress_bar_bg);
    }

    const bool hovered = opt->state & QStyle::State_MouseOver;
    QColor hc          = hovered ? palette_.highlight.lighter(120) : palette_.highlight;
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

void WusicProxyStyle::draw_scroll_bar(ComplexControl /*cc*/, const QStyleOptionComplex* opt,
                                      QPainter* p, const QWidget* w) const
{
    const auto* sb = qstyleoption_cast<const QStyleOptionSlider*>(opt);
    if (!sb) {
        return;
    }

    const bool horiz = sb->orientation == Qt::Horizontal;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // 1) 整条滑槽背景
    p->fillRect(opt->rect, palette_.scroll_bar_bg);

    // 2) 箭头按钮（绘制小三角，隐藏 Fusion 默认按钮）
    auto draw_arrow_btn = [&](SubControl sc, bool is_up_or_left) {
        QRect r = QProxyStyle::subControlRect(CC_ScrollBar, sb, sc, w);
        if (r.isNull()) {
            return;
        }
        // 按钮背景略亮于滑槽
        p->fillRect(r, palette_.button);
        // 三角形
        int cx = r.center().x();
        int cy = r.center().y();
        p->setPen(palette_.text);
        p->setBrush(palette_.text);
        QPainterPath tri;
        if (horiz) {
            if (is_up_or_left) {
                tri.moveTo(cx - 3, cy);
                tri.lineTo(cx + 3, cy - 3);
                tri.lineTo(cx + 3, cy + 3);
            } else {
                tri.moveTo(cx + 3, cy);
                tri.lineTo(cx - 3, cy - 3);
                tri.lineTo(cx - 3, cy + 3);
            }
        } else {
            if (is_up_or_left) {
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
    draw_arrow_btn(SC_ScrollBarSubLine, true);
    draw_arrow_btn(SC_ScrollBarAddLine, false);

    // 3) 滑块
    QRect sr = QProxyStyle::subControlRect(CC_ScrollBar, sb, SC_ScrollBarSlider, w);
    if (!sr.isNull()) {
        const bool hovered = opt->state & QStyle::State_MouseOver;
        QColor handle_color =
            hovered ? palette_.scroll_bar_handle.lighter(130) : palette_.scroll_bar_handle;
        if (horiz) {
            sr.adjust(0, 2, 0, -2);
        } else {
            sr.adjust(2, 0, -2, 0);
        }
        p->fillRect(sr, handle_color);
    }

    p->restore();
}

// ============================================================================
// drawMenuItem
// ============================================================================

void WusicProxyStyle::draw_menu_item(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
    if (!mi || mi->menuItemType == QStyleOptionMenuItem::Separator) {
        if (mi) {
            p->setPen(QPen(palette_.frame_border, 1));
            p->drawLine(opt->rect.left() + 8, opt->rect.center().y(), opt->rect.right() - 8,
                        opt->rect.center().y());
        }
        return;
    }

    const bool sel = opt->state & QStyle::State_Selected;
    const bool ena = opt->state & QStyle::State_Enabled;

    if (sel && ena) {
        fill_round(p, opt->rect.adjusted(2, 1, 2, -1), palette_.highlight, palette_.menu_radius);
    }

    QStyleOptionMenuItem copy = *mi;
    copy.palette.setColor(QPalette::Text, sel ? palette_.highlighted_text : palette_.text);
    copy.palette.setColor(QPalette::HighlightedText, palette_.highlighted_text);

    if (!mi->icon.isNull()) {
        QRect ir = opt->rect;
        ir.setWidth(ir.height());
        mi->icon.paint(p, ir.adjusted(4, 4, -4, -4));
    }

    // QComboBox 的弹出列表由 QComboMenuDelegate 以"菜单项"方式绘制
    // (Qt qcombobox.cpp: QComboMenuDelegate::getStyleOption), 其 maxIconWidth
    // 恒为 view decorationSize+4(默认 PM_ListViewIconSize=24 → 28), 即使选项
    // 并没有图标。若照常预留 maxIconWidth+8=36px 图标列, 会把与闭合态等宽的
    // 窄 popup 中的文本向右挤压、后半截被截断。此处对 combo 弹出项且无图标时
    // 只保留常规 8px 左边距; 有图标的 combo 项(或普通 QMenu)仍按原逻辑对齐。
    int left_pad = mi->maxIconWidth + 8;
    if (qobject_cast<const QComboBox*>(w) && mi->icon.isNull()) {
        left_pad = 8;
    }

    QRect tr = opt->rect.adjusted(left_pad, 0, -12, 0);
    drawItemText(p, tr, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, copy.palette, ena,
                 mi->text, sel ? QPalette::HighlightedText : QPalette::Text);
}

// ============================================================================
// drawMenuBarItem
// ============================================================================

void WusicProxyStyle::draw_menu_bar_item(const QStyleOption* opt, QPainter* p) const
{
    const bool sel = opt->state & QStyle::State_Selected;
    const bool ena = opt->state & QStyle::State_Enabled;

    if (sel && ena) {
        fill_round(p, opt->rect.adjusted(2, 2, -2, -2), palette_.item_hover,
                   palette_.button_radius);
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

void WusicProxyStyle::draw_header(const QStyleOption* opt, QPainter* p) const
{
    const auto* hdr    = qstyleoption_cast<const QStyleOptionHeader*>(opt);
    const bool hovered = opt->state & QStyle::State_MouseOver;
    const bool sunken  = opt->state & QStyle::State_Sunken;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // 背景
    QColor bg = palette_.button;
    if (sunken) {
        bg = palette_.highlight;
    } else if (hovered) {
        bg = palette_.item_hover;
    }
    fill_round(p, opt->rect, bg, palette_.button_radius);

    // 底部边框
    p->setPen(QPen(palette_.frame_border, 1));
    p->drawLine(opt->rect.bottomLeft(), opt->rect.bottomRight());

    // 列间分隔线（右边缘），用 splitterHandle 颜色与底部边框区分
    p->setPen(QPen(palette_.splitter_handle, 1));
    p->drawLine(opt->rect.topRight() + QPoint(0, 4), opt->rect.bottomRight() + QPoint(0, -4));

    // 文字 + 排序箭头
    if (hdr) {
        QStyleOptionHeader copy = *hdr;
        copy.palette.setColor(QPalette::Text, palette_.text);
        copy.palette.setColor(QPalette::WindowText, palette_.text);
        copy.palette.setColor(QPalette::ButtonText, palette_.text);

        // 文字
        if (!hdr->text.isEmpty()) {
            QRect text_rect = opt->rect.adjusted(4, 0, -4, 0);
            // 有排序箭头时右侧留空
            if (hdr->sortIndicator != QStyleOptionHeader::None) {
                text_rect.setRight(text_rect.right() - 16);
            }
            const int text_flags = static_cast<int>(hdr->textAlignment) |
                                   static_cast<int>(Qt::AlignVCenter) |
                                   static_cast<int>(Qt::TextShowMnemonic);
            drawItemText(p, text_rect, text_flags, copy.palette, true, hdr->text, QPalette::Text);
        }

        // 排序箭头：紧贴右边缘，QPainter 直接绘制
        if (hdr->sortIndicator != QStyleOptionHeader::None) {
            const int cx  = opt->rect.right() - 8; // 箭头中心 X，距右边缘 8px
            const int cy  = opt->rect.center().y();
            const bool up = (hdr->sortIndicator == QStyleOptionHeader::SortUp);

            p->setPen(QPen(palette_.text, 1.5));
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

void WusicProxyStyle::draw_combo_box(const QStyleOptionComplex* opt, QPainter* p) const
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

    QColor bg          = ena ? palette_.base : palette_.base.darker(120);
    QColor bd          = hovered ? palette_.highlight : palette_.frame_border;

    fill_round(p, opt->rect.adjusted(1, 1, -1, -1), bg, palette_.button_radius);
    stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), bd, palette_.button_radius);

    QRect arrow = QProxyStyle::subControlRect(CC_ComboBox, cb, SC_ComboBoxArrow, nullptr);
    if (!arrow.isNull()) {
        QPainterPath ap;
        int cx = arrow.center().x();
        int cy = arrow.center().y();
        ap.moveTo(cx - 4, cy - 2);
        ap.lineTo(cx, cy + 3);
        ap.lineTo(cx + 4, cy - 2);
        ap.closeSubpath();
        p->setBrush(palette_.text);
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

void WusicProxyStyle::draw_spin_box(const QStyleOptionComplex* opt, QPainter* p,
                                    const QWidget* w) const
{
    const auto* sb = qstyleoption_cast<const QStyleOptionSpinBox*>(opt);
    if (!sb) {
        QProxyStyle::drawComplexControl(CC_SpinBox, opt, p, w);
        return;
    }

    p->save();

    // 1) 背景 + 边框
    fill_round(p, opt->rect.adjusted(1, 1, -1, -1), palette_.base, palette_.button_radius);
    stroke_round(p, opt->rect.adjusted(1, 1, -1, -1), palette_.frame_border,
                 palette_.button_radius);

    // 2) 上下按钮背景
    QRect up_rect   = QProxyStyle::subControlRect(CC_SpinBox, sb, SC_SpinBoxUp, w);
    QRect down_rect = QProxyStyle::subControlRect(CC_SpinBox, sb, SC_SpinBoxDown, w);
    if (!up_rect.isNull()) {
        p->fillRect(up_rect.adjusted(1, 1, 0, 0), palette_.button);
    }
    if (!down_rect.isNull()) {
        p->fillRect(down_rect.adjusted(1, 0, 0, -1), palette_.button);
    }

    // 3) 箭头（在按钮背景之上）
    if (!up_rect.isNull()) {
        QStyleOption up_opt;
        up_opt.rect = up_rect;
        drawPrimitive(PE_IndicatorSpinUp, &up_opt, p, w);
    }
    if (!down_rect.isNull()) {
        QStyleOption down_opt;
        down_opt.rect = down_rect;
        drawPrimitive(PE_IndicatorSpinDown, &down_opt, p, w);
    }

    p->restore();
}

// ============================================================================
// drawTabBarTab
// ============================================================================

void WusicProxyStyle::draw_tab_bar_bab(const QStyleOption* opt, QPainter* p, const QWidget* w) const
{
    const bool sel     = opt->state & QStyle::State_Selected;
    const bool hovered = opt->state & QStyle::State_MouseOver;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    QColor bg = palette_.window;
    if (sel) {
        bg = palette_.base;
    } else if (hovered) {
        bg = palette_.item_hover;
    }
    fill_round(p, opt->rect.adjusted(2, 2, -2, opt->rect.height() / 2), bg, palette_.button_radius);

    if (sel) {
        p->fillRect(QRect(opt->rect.left() + 4, opt->rect.bottom() - 2, opt->rect.width() - 8, 2),
                    palette_.highlight);
    }

    // QTabBar 只调用 CE_TabBarTab，不会单独调用 CE_TabBarTabLabel，
    // 因此必须在这里显式委托绘制标签文字 / 图标 / 关闭按钮，否则文字不会显示。
    drawControl(CE_TabBarTabLabel, opt, p, w);

    p->restore();
}

// ============================================================================
// drawCheckIndicator
// ============================================================================

void WusicProxyStyle::draw_check_indicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r      = opt->rect.adjusted(2, 2, -2, -2);
    const bool checked = opt->state & QStyle::State_On;
    const bool enabled = opt->state & QStyle::State_Enabled;

    QColor bg          = checked ? palette_.highlight : palette_.base;
    QColor bd          = enabled ? palette_.frame_border : palette_.frame_border.darker(150);

    fill_round(p, r, bg, palette_.button_radius);
    stroke_round(p, r, bd, palette_.button_radius);

    if (checked) {
        QPen pen(palette_.highlighted_text, 2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p->setPen(pen);
        QPainterPath check;
        int cx = r.center().x();
        int cy = r.center().y();
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

void WusicProxyStyle::draw_radio_indicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const QRect r      = opt->rect.adjusted(2, 2, -2, -2);
    const bool checked = opt->state & QStyle::State_On;
    const bool enabled = opt->state & QStyle::State_Enabled;

    QColor bd;
    if (!enabled) {
        bd = palette_.frame_border.darker(150);
    } else {
        bd = checked ? palette_.highlight : palette_.frame_border;
    }

    p->setPen(QPen(bd, 2));
    p->setBrush(palette_.base);
    p->drawEllipse(r);

    if (checked) {
        p->setPen(Qt::NoPen);
        p->setBrush(palette_.highlight);
        p->drawEllipse(r.adjusted(3, 3, -3, -3));
    }

    p->restore();
}

// ============================================================================
// drawBranchIndicator
// ============================================================================

void WusicProxyStyle::draw_branch_indicator(const QStyleOption* opt, QPainter* p) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    const int cx = opt->rect.center().x();
    const int cy = opt->rect.center().y();

    p->setPen(QPen(palette_.text, 1.5));
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
