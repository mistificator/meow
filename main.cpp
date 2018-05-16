#include "mainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QDebug>
#include <QLibrary>
#include <QMessageBox>
#include "settingspage.h"

typedef void (* WTFCrashHookFunction)();
typedef void (* WTFSetCrashHookFunction)(WTFCrashHookFunction);
void WTFCrashHook()
{
    qDebug() << "Webkit BAD BEEF!!!";
    QMessageBox::critical(0, "Fatal WebKit error", "Application will be closed");
}

int main(int argc, char *argv[])
{
    QSharedMemory shmem("meow_single");
    if (!shmem.create(1))
    {
        shmem.attach();
        qDebug() << "already running";
        shmem.detach();
        return 0;
    }
    shmem.attach();

#if defined(Q_OS_WIN) && defined(_DEBUG)
    WTFSetCrashHookFunction WTFSetCrashHook = (WTFSetCrashHookFunction)QLibrary::resolve("Qt5WebKitd", "WTFSetCrashHook");
    if (WTFSetCrashHook)
    {
        WTFSetCrashHook(WTFCrashHook);
    }
#endif

    QApplication a(argc, argv);

    if (!SettingsPage::itWasSuccessExit() && SettingsPage::startPage() == SettingsPage::PageLast)
    {
        SettingsPage::setLastPages(QStringList());
    }
    SettingsPage::setItWasSuccessExit(false);

    MainWindow w;
    w.show();
    int code = a.exec();

    SettingsPage::setItWasSuccessExit(code == 0);

    shmem.detach();

    return 0;
}
