#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Player player;
    MainWindow w(&player);
    
    w.show();
    return a.exec();
}
