#include "MainWindow.h"

#include <QApplication>
#include "src/ConfigManager/ConfigManager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Player player;
    MainWindow w(&player);
    
    w.show();
    return a.exec();
}
