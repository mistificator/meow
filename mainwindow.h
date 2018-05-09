#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QScrollArea>
#include "webpage.h"
#include "settingspage.h"
#include "downloadspage.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionAddTab_triggered();
    void on_actionAddIncognitoTab_triggered();
    void on_tabWidget_tabCloseRequested(int index);
    void newWindowLink(QUrl);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);
    void showNotifier(QString, int timeout = 5000);
    void hideNotifier();
    void on_actionShowSettings_triggered();
    void proxyChanged(SettingsPage::ProxyType);
    void privacyChanged(SettingsPage::Privacy);
    void on_actionShowDownloads_triggered();
    void tabCustomContextMenuRequested(const QPoint &pos);
    void on_actionSavePage_triggered();

    void on_actionDownloadPage_triggered();

    void on_actionRunScript_triggered();

private:
    Ui::MainWindow *ui;
    QString temp_dir;
    WebPage * last_page;
    QMenu newtab_menu;
    QLabel * notifier;
    QTimer * notifier_timer;
    QScrollArea * settings_page;
    DownloadsPage * downloads_page;
    QToolButton * add_tab;
    void openLastPages();
    void saveLastPages();
    void openHomePage();
    void openEmptyPage();
};

#endif // MAINWINDOW_H
