#pragma once

#include <QObject>
#include <memory>

class PlaybackController;
class MainWindow;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playbackController, QObject* parent = nullptr);
    ~AppController() override;

    void showMainWindow();

private:
    PlaybackController* m_playbackController = nullptr;
    std::unique_ptr<MainWindow> m_mainWindow;
};
