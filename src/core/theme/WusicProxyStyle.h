#pragma once

#include <QProxyStyle>
#include "ThemePalette.h"

/// 完整的自定义绘制样式，基于 QProxyStyle(Fusion)。
/// 覆写了所有常见控件的 drawControl / drawPrimitive / drawComplexControl。
/// 未覆写的极少数元素委托给基础 Fusion 样式。
///
/// 调参/调试流程：
///   1. 修改 ThemePalette 中的颜色/尺寸 → 全局生效
///   2. 修改本文件中对应 case 的绘制逻辑 → 精确控制单个控件外观
class WusicProxyStyle : public QProxyStyle {
public:
    explicit WusicProxyStyle(const ThemePalette& palette,
                             const QString& baseKey = QStringLiteral("Fusion"));

    void setPalette(const ThemePalette& palette);
    const ThemePalette& palette() const { return m_palette; }

    // ---- QStyle 必须覆写的接口 ----
    QPalette standardPalette() const override;

    void drawControl(ControlElement element, const QStyleOption* opt,
                     QPainter* p, const QWidget* w) const override;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                       QPainter* p, const QWidget* w) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt,
                            QPainter* p, const QWidget* w) const override;

    int pixelMetric(PixelMetric m, const QStyleOption* opt,
                    const QWidget* w) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                         SubControl sc, const QWidget* w) const override;

    void polish(QWidget* w) override;
    void unpolish(QWidget* w) override;
    int styleHint(StyleHint hint, const QStyleOption* opt,
                  const QWidget* w, QStyleHintReturn* ret) const override;

private:
    ThemePalette m_palette;

    // ---- 控件级绘制 ----
    void drawButton(const QStyleOption* opt, QPainter* p) const;
    void drawSlider(const QStyleOption* opt, QPainter* p) const;
    void drawScrollBar(ComplexControl cc, const QStyleOptionComplex* opt,
                       QPainter* p, const QWidget* w) const;
    void drawMenuItem(const QStyleOption* opt, QPainter* p) const;
    void drawMenuBarItem(const QStyleOption* opt, QPainter* p) const;
    void drawHeader(const QStyleOption* opt, QPainter* p) const;
    void drawComboBox(const QStyleOptionComplex* opt, QPainter* p) const;
    void drawTabBarTab(const QStyleOption* opt, QPainter* p) const;
    void drawSpinBox(const QStyleOptionComplex* opt, QPainter* p, const QWidget* w) const;
    void drawToolButton(const QStyleOptionComplex* opt, QPainter* p) const;

    // ---- 图元级绘制 ----
    void drawCheckIndicator(const QStyleOption* opt, QPainter* p) const;
    void drawRadioIndicator(const QStyleOption* opt, QPainter* p) const;
    void drawBranchIndicator(const QStyleOption* opt, QPainter* p) const;

    // ---- 通用辅助 ----
    void fillRound(QPainter* p, const QRect& r, const QColor& bg, int radius) const;
    void strokeRound(QPainter* p, const QRect& r, const QColor& border, int radius, int w = 1) const;
};
