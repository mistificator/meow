#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QScrollArea>
#include "webpage.h"

class WebPage;

namespace Ui {
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
    void on_tabWidget_tabCloseRequested(int index);
    void newWindowLink(QUrl);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);
    void showNotifier(QString, int timeout = 5000);
    void hideNotifier();
    void on_actionShowSettings_triggered();

private:
    Ui::MainWindow *ui;
    QString temp_dir;
    WebPage * last_page;
    QMenu newtab_menu;
    QLabel * notifier;
    QTimer * notifier_timer;
    QScrollArea * settings_page;
};

#endif // MAINWINDOW_H
