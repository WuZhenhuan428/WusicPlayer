#pragma once

#include <QFontMetrics>
#include <QLabel>
#include <QObject>
#include <QResizeEvent>
#include <QString>

class ElidedLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString full_text READ full_text WRITE set_full_text)
public:
    explicit ElidedLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr) : QLabel(parent)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        this->set_full_text(text);
    }

    [[nodiscard]] QString full_text() const
    {
        return full_text_;
    }

public slots:
    void set_full_text(const QString& text)
    {
        if (full_text_ != text) {
            full_text_ = text;
            update_elided_text();
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QLabel::resizeEvent(event);
        update_elided_text();
    }

private:
    void update_elided_text()
    {
        QFontMetrics metrics(font());
        const int avaliable_width = qMax(0, this->contentsRect().width());
        QString elided_text       = metrics.elidedText(full_text_, Qt::ElideRight, avaliable_width);
        if (elided_text != text()) {
            QLabel::setText(elided_text);
            setToolTip(full_text_ != elided_text ? full_text_ : "");
        }
    }

    QString full_text_;
};
