#include "playlist_widgets.h"

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
    // input title
    lbTitle = new QLabel("Title:");
    txtTitle = new QLineEdit(this);
    hbTitle = new QHBoxLayout();
    hbTitle->addWidget(lbTitle);
    hbTitle->addWidget(txtTitle);

    // choose type
    lbType = new QLabel("Type:");
    cbType = new QComboBox();
    QVector<QString> types = { QString(), // <- null
        "title", "artist", "album", "album artist",
        "genre", "composer", "year", "date", "track",
        "track_number", "disc", "disc_number", "bitrate",
        "filename", "directory"
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
    vbMain->addLayout(hbTitle);
    vbMain->addLayout(hbType);
    vbMain->addStretch();
    vbMain->addLayout(hbBtn);
    this->setLayout(vbMain);
    
    connect(btnOK, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

WInsertColumnDialog::~WInsertColumnDialog() {}

TableColumn WInsertColumnDialog::getRule() {
    TableColumn retval;
    retval.headerName = txtTitle->text();
    retval.sortType = mapStrToSorttype.value(cbType->currentText(), SortType::not_sorted);
    return retval;
}
