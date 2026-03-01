#include "playlist_widgets.h"

#include <limits>

// ==== WSortTypeSetDialog ==== //
WSortTypeSetDialog::WSortTypeSetDialog(QDialog *parent)
    : QDialog(parent)
{
    lbPrompt = new QLabel("Input sorting expression:");
    txtExpression = new QLineEdit(this);
    btnEnter = new QPushButton("OK");
    btnCancel = new QPushButton("Cancel");
    btnHelp = new QPushButton("Help");
    sldMain = new QSlider();
    sldBtn = new QSlider();
    hbPrompt = new QHBoxLayout();
    hbBtn = new QHBoxLayout();
    vbMain = new QVBoxLayout();

    connect(btnEnter, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnHelp, &QPushButton::clicked, this, [this](){
        QMessageBox::information(this, "foo", "bar");
    });

    hbPrompt->addWidget(lbPrompt);
    hbPrompt->addStretch();

    hbBtn->addWidget(btnHelp);
    hbBtn->addStretch();
    hbBtn->addWidget(btnCancel);
    hbBtn->addWidget(btnEnter);

    vbMain->addSpacing(5);
    vbMain->addLayout(hbPrompt);
    vbMain->addWidget(txtExpression);
    vbMain->addStretch();
    vbMain->addLayout(hbBtn);
    
    this->setLayout(vbMain);
}

WSortTypeSetDialog::~WSortTypeSetDialog() {};


// ==== WInsertColumnDialog ==== //

WInsertColumnDialog::WInsertColumnDialog() {
    // index
    lbIndex = new QLabel("Index:");
    txtIndex = new QLineEdit(this);
    txtIndex->setText("1");
    hbIndex = new QHBoxLayout();
    hbIndex->addWidget(lbIndex);
    hbIndex->addWidget(txtIndex);

    // input title
    lbTitle = new QLabel("Title:");
    txtTitle = new QLineEdit(this);
    hbTitle = new QHBoxLayout();
    hbTitle->addWidget(lbTitle);
    hbTitle->addWidget(txtTitle);

    // choose type
    lbType = new QLabel("Type:");
    cbType = new QComboBox();
    QVector<QString> types = {
        "title", "artist", "album", "album artist",
        "genre", "composer", "year", "date", "track",
        "disc","bitrate", "filename", "directory"
    };
    cbType->addItems(types);
    hbType = new QHBoxLayout();
    hbType->addWidget(lbType);
    hbType->addWidget(cbType);

    // button
    btnOK = new QPushButton("OK");
    btnCancel = new QPushButton("Cancel");
    hbBtn = new QHBoxLayout();
    hbBtn->addStretch();
    hbBtn->addWidget(btnOK);
    hbBtn->addWidget(btnCancel);

    // main layout
    vbMain = new QVBoxLayout();
    vbMain->addLayout(hbIndex);
    vbMain->addLayout(hbTitle);
    vbMain->addLayout(hbType);
    vbMain->addStretch();
    vbMain->addLayout(hbBtn);
    this->setLayout(vbMain);
    
    connect(btnOK, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        qlonglong value = txtIndex ? txtIndex->text().toLongLong(&ok) : 0;
        if (!ok || value <= 0) {
            QMessageBox::warning(this, "Invalid index", "Index must be a positive integer (not 0).");
            return;
        }
        if (value > std::numeric_limits<int>::max()) {
            QMessageBox::warning(this, "Invalid index", "Index is too large.");
            return;
        }
        int intValue = static_cast<int>(value);
        if (m_maxIndex > 0 && intValue > m_maxIndex) {
            QMessageBox::warning(this, "Invalid index", "Index is out of range. Clamped to max.");
            txtIndex->setText(QString::number(m_maxIndex));
            accept();
            return;
        }
        accept();
    });
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

WInsertColumnDialog::~WInsertColumnDialog() {}

void WInsertColumnDialog::setIndex(int index) {
    if (txtIndex) {
        txtIndex->setText(QString::number(index));
    }
}

void WInsertColumnDialog::setMaxIndex(int index) {
    m_maxIndex = index;
}

int WInsertColumnDialog::index() const {
    bool ok = false;
    int value = txtIndex ? txtIndex->text().toInt(&ok) : 0;
    return ok ? value : 0;
}

TableColumn WInsertColumnDialog::getRule() {
    TableColumn retval;
    retval.headerName = txtTitle->text();
    retval.sortType = mapStrToSorttype.value(cbType->currentText(), SortType::not_sorted);
    return retval;
}

// ==== WColumnIndexDialog ==== //
WColumnIndexDialog::WColumnIndexDialog(const QString& title, const QString& prompt, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    lbPrompt = new QLabel(prompt);
    txtIndex = new QLineEdit(this);
    txtIndex->setText("1");
    hbIndex = new QHBoxLayout();
    hbIndex->addWidget(lbPrompt);
    hbIndex->addWidget(txtIndex);

    btnOK = new QPushButton("OK");
    btnCancel = new QPushButton("Cancel");
    hbBtn = new QHBoxLayout();
    hbBtn->addStretch();
    hbBtn->addWidget(btnOK);
    hbBtn->addWidget(btnCancel);

    vbMain = new QVBoxLayout();
    vbMain->addLayout(hbIndex);
    vbMain->addStretch();
    vbMain->addLayout(hbBtn);
    setLayout(vbMain);

    connect(btnOK, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        qlonglong value = txtIndex ? txtIndex->text().toLongLong(&ok) : 0;
        if (!ok || value <= 0) {
            QMessageBox::warning(this, "Invalid index", "Index must be a positive integer (not 0).");
            return;
        }
        if (value > std::numeric_limits<int>::max()) {
            QMessageBox::warning(this, "Invalid index", "Index is too large.");
            return;
        }
        int intValue = static_cast<int>(value);
        if (m_maxIndex > 0 && intValue > m_maxIndex) {
            QMessageBox::warning(this, "Invalid index", "Index is out of range. Clamped to max.");
            txtIndex->setText(QString::number(m_maxIndex));
            accept();
            return;
        }
        accept();
    });
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

WColumnIndexDialog::~WColumnIndexDialog() {}

void WColumnIndexDialog::setIndex(int index) {
    if (txtIndex) {
        txtIndex->setText(QString::number(index));
    }
}

void WColumnIndexDialog::setMaxIndex(int index) {
    m_maxIndex = index;
}

int WColumnIndexDialog::index() const {
    bool ok = false;
    int value = txtIndex ? txtIndex->text().toInt(&ok) : 0;
    return ok ? value : 0;
}
