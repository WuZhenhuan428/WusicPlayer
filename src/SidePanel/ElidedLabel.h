#pragma once

#include <QLabel>
#include <QResizeEvent>
#include <QObject>
#include <QString>
#include <QFontMetrics>

class ElidedLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString fullText READ fullText WRITE setFullText)
public:
    explicit ElidedLabel(QWidget* parent = nullptr) : QLabel(parent) {}
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(parent) {
        this->setText(text);
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    QString fullText() const { return m_fullText; }

public slots:
    void setFullText(const QString& text) {
        if (m_fullText != text) {
            m_fullText = text;
            updateElidedText();
        }
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        updateElidedText();
    }

private:
    void updateElidedText() {
        QFontMetrics metrics(font());
        int avaliable_width = width() - contentsMargins().left() - contentsMargins().right();
        QString elided_text = metrics.elidedText(m_fullText, Qt::ElideRight, avaliable_width);
        if (elided_text != text()) {
            QLabel::setText(elided_text);
            setToolTip(m_fullText != elided_text ? m_fullText : "");
        }
    }

    QString m_fullText;
};