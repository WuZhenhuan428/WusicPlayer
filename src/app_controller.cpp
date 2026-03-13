#include "app_controller.h"

#include <QCoreApplication>

#include "view/MainWindow.h"
#include "controller/PlaybackController.h"

AppController::AppController(PlaybackController* playbackController, QObject* parent)
    : QObject(parent),
      m_playbackController(playbackController),
      m_mainWindow(std::make_unique<MainWindow>(m_playbackController))
{
    m_mainWindow->initializeAfterConstruction();

    connect(m_mainWindow.get(), &MainWindow::sgnAboutToClose,
            m_mainWindow.get(), &MainWindow::persistState);

    connect(qApp, &QCoreApplication::aboutToQuit,
            m_mainWindow.get(), &MainWindow::persistState);
}

AppController::~AppController() = default;

void AppController::showMainWindow()
{
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}
