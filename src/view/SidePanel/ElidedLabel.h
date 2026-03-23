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
    explicit ElidedLabel(QWidget* parent = nullptr) : QLabel(parent) {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(parent) {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        this->setFullText(text);
    }

    QString fullText() const { return m_full_text; }

public slots:
    void setFullText(const QString& text) {
        if (m_full_text != text) {
            m_full_text = text;
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
        const int avaliable_width = qMax(0, this->contentsRect().width());
        QString elided_text = metrics.elidedText(m_full_text, Qt::ElideRight, avaliable_width);
        if (elided_text != text()) {
            QLabel::setText(elided_text);
            setToolTip(m_full_text != elided_text ? m_full_text : "");
        }
    }

    QString m_full_text;
};