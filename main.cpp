#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // from https://habr.com/post/188816/
    QStringList paths = QApplication::libraryPaths();
    paths.append(".");
    paths.append("imageformats");
    paths.append("platforms");
    paths.append("sqldrivers");
    paths.append("audio");
    paths.append("mediaservice");
    paths.append("printsupport");
    QApplication::setLibraryPaths(paths);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
