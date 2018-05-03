#include "mainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QDebug>

int main(int argc, char *argv[])
{
    QSharedMemory shmem("meow_single");
    if (!shmem.create(1))
    {
        qDebug() << "already running";
        return 0;
    }

    // from https://habr.com/post/188816/
    QApplication::setLibraryPaths(QStringList() << qApp->applicationDirPath());

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
