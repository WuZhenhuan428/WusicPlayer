#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include "player.h"

#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

// TODO: complete list widgets
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableView>
#include <QLabel>
#include <QListView>
#include <QPixmap>

#include "src/playlist/playlist_manager.h"
#include "wtimeprogress.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Player* player, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Player* m_player;
    PlaylistManager* m_playlistManager;
    void initUI();
    void initConnection();

    // UI Action
    void onOpenFile();
    void onLoadPlaylist();
    void onSaveCurrPlaylist();

    // UI Widgets declaraion
    /// Menu widgets
    QMenuBar* mainMenuBar;
    QToolBar* bottomToolBar;
    // QToolBar* leftToolbar;
    QMenu* menuFile;
    QMenu* menuHelp;
    QAction* actOpenFile;
    QAction* actLoadPlaylist;
    QAction* actSaveCurrPlaylist;
    QAction* actExit;
    QAction* actManual;
    QAction* actAbout;

    /// Control widgets
    QPushButton* btnPlay;
    QPushButton* btnPause;
    QPushButton* btnStop;
    QPushButton* btnMute;

    /// Progress Bar: Position/Duration
    QSlider* sliderPostion;
    WTimeProgress* timeProgress;
    QSlider* sliderVolume;

    // main window:
    /// Playlist | song table | cover & rolling lyrics
    QSplitter* mainSplitter;
    QTreeWidget* playlistTree;
    QTableView* songTableView;

    QSplitter* coverSplitter;
    QLabel* coverImageLabel;
    QListView* lrcListView; // Temporarily used for placeholder purposes

    // Communication with Player
    void onPlayerStateChanged(Player::State state);

private slots:
    void updateDuration(qint64 duration_ms);
    void updatePosition(qint64 position_ms);
    // void changeSongTable();
    
signals:
    void filepathChanged(QString filepath);
    void loadPlaylist(QString filepath);
};
#endif // MAINWINDOW_H
