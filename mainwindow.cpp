#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tabbar.h"
#include <QToolButton>
#include <QLabel>
#include <QDir>
#include <QtWebKit/QWebSettings>
#include <QApplication>
#include <QtWebKitWidgets/QWebView>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSslConfiguration>
#include <QSslSocket>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    last_page(0), settings_page(0), downloads_page(0)
{
    proxyChanged(SettingsPage::proxyType());

    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
//    QWebSettings::globalSettings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);

    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setPeerVerifyMode(QSslSocket::QueryPeer);
    QSslConfiguration::setDefaultConfiguration(conf);

    ui->setupUi(this);
    ui->mainToolBar->hide();

    notifier = new QLabel(this);
    notifier->hide();
    notifier_timer = new QTimer(this);
    notifier_timer->setSingleShot(true);

    setWindowIcon(QIcon(":/Images/16x16.png"));
    ui->actionAddTab->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    ui->actionAddIncognitoTab->setIcon(style()->standardIcon(QStyle::SP_VistaShield));
    ui->actionShowSettings->setIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));
    ui->actionShowDownloads->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));

    newtab_menu.addAction(ui->actionAddTab);
    newtab_menu.addAction(ui->actionAddIncognitoTab);
    newtab_menu.addSeparator();
    newtab_menu.addAction(ui->actionShowSettings);
    newtab_menu.addAction(ui->actionShowDownloads);

    // taken from https://stackoverflow.com/questions/19975137/how-can-i-add-a-new-tab-button-next-to-the-tabs-of-a-qmdiarea-in-tabbed-view-m/27127109#27127109
    add_tab = new QToolButton();
    add_tab->setToolButtonStyle(Qt::ToolButtonIconOnly);
    add_tab->setPopupMode(QToolButton::MenuButtonPopup);
    add_tab->setDefaultAction(SettingsPage::privacyPolicy() == SettingsPage::UsualPrivacy ? ui->actionAddTab : ui->actionAddIncognitoTab);
    add_tab->setMenu(& newtab_menu);
    // Set custom tab bar
    bool movable = ui->tabWidget->isMovable();
    bool closable = ui->tabWidget->tabsClosable();
    TabWidgetOpener::setTabBar(ui->tabWidget, new TabBar(this));
    ui->tabWidget->setMovable(movable);
    ui->tabWidget->setTabsClosable(closable);
    // Add empty, not enabled tab to tabWidget
    ui->tabWidget->addTab(new QWidget(), QString());
    ui->tabWidget->setTabEnabled(0, false);
    // Add tab button to current tab. Button will be enabled, but tab -- not
    TabWidgetOpener::tabBar(ui->tabWidget)->setTabButton(0, QTabBar::RightSide, add_tab);

    QWebSettings::enablePersistentStorage(SettingsPage::tempDir());
    SettingsPage::loadScripts();

    switch (SettingsPage::startPage())
    {
    case SettingsPage::PageEmpty:
        openEmptyPage();
        break;
    case SettingsPage::PageHome:
        openHomePage();
        break;
    case SettingsPage::PageLast:
        openLastPages();
        break;
    }
}

MainWindow::~MainWindow()
{
    saveLastPages();
    delete ui;
}

void MainWindow::on_actionAddTab_triggered()
{
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->insertTab(index, last_page = new WebPage(WebPage::UsualPrivacy, this), QString());
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, last_page->windowIcon());

    connect(last_page, SIGNAL(newWindowLink(QUrl)), this, SLOT(newWindowLink(QUrl)));
    connect(last_page, SIGNAL(linkHovered(QString, QString, QString)), this, SLOT(linkHovered(QString, QString, QString)));
}

void MainWindow::on_actionAddIncognitoTab_triggered()
{
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->insertTab(index, last_page = new WebPage(WebPage::IncognitoPrivacy, this), QString());
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, last_page->windowIcon());

    connect(last_page, SIGNAL(newWindowLink(QUrl)), this, SLOT(newWindowLink(QUrl)));
    connect(last_page, SIGNAL(linkHovered(QString, QString, QString)), this, SLOT(linkHovered(QString, QString, QString)));
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    showNotifier(ui->tabWidget->tabText(index).trimmed() + " closed");

    QWidget * w = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    if (WebPage * page = dynamic_cast<WebPage *>(w))
    {
        page->view()->load(QUrl("about:blank"));
    }
    if (w == settings_page)
    {
        settings_page = 0;
    }
    if (w == downloads_page)
    {
        downloads_page = 0;
    }
    w->deleteLater();

    ui->tabWidget->setCurrentIndex(qMax(index - 1, 0));
}

void MainWindow::newWindowLink(QUrl url)
{
    bool incognito = false;
    if (WebPage * webpage = dynamic_cast<WebPage *>(sender()))
    {
        incognito = webpage->isIncognito();
    }
    if (!incognito)
    {
        ui->actionAddTab->trigger();
    }
    else
    {
        ui->actionAddIncognitoTab->trigger();
    }
    last_page->view()->load(url);
}

void MainWindow::linkHovered(const QString & link, const QString &, const QString &)
{
    ui->statusBar->showMessage(link, 10000);
}

void MainWindow::showNotifier(QString message, int timeout)
{
// based on https://stackoverflow.com/questions/43133884/qt5-show-notification-popups
    QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect();
    notifier->setGraphicsEffect(effect);
    notifier->setStyleSheet("border: 3px solid gray;border-radius:20px;background-color:#ffffff;color:gray");
    notifier->setAlignment(Qt::AlignCenter);
    notifier->setText(message);
    notifier->resize(QFontMetrics(notifier->font()).width(message) + 20, notifier->height());
    QPoint pt = QPoint(ui->centralWidget->rect().right(), ui->centralWidget->rect().bottom());// = mapFrom(ui->tabWidget, QPoint(ui->tabWidget->rect().right(), ui->tabWidget->rect().bottom()));
    notifier->move(pt.x() - notifier->width(), pt.y() - notifier->height());
    QPropertyAnimation * a = new QPropertyAnimation(effect,"opacity");
    a->setDuration(1000);  // in miliseconds
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    notifier->show();
    connect(notifier_timer, SIGNAL(timeout()), this, SLOT(hideNotifier()));
    notifier_timer->start(timeout);
}

void MainWindow::hideNotifier()
{
// based on https://stackoverflow.com/questions/43133884/qt5-show-notification-popups
    QGraphicsOpacityEffect * effect = new QGraphicsOpacityEffect();
    notifier->setGraphicsEffect(effect);
    QPropertyAnimation * a = new QPropertyAnimation(effect,"opacity");
    a->setDuration(1000); // it will took 1000ms to face out
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::OutBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, SIGNAL(finished()), notifier, SLOT(hide()));
    connect(a, SIGNAL(finished()), notifier_timer, SLOT(stop()));
}

void MainWindow::on_actionShowSettings_triggered()
{
    if (settings_page)
    {
        ui->tabWidget->setCurrentWidget(settings_page);
        return;
    }
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->insertTab(index, settings_page = new QScrollArea(this), "Settings");
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, style()->standardIcon(QStyle::SP_DriveNetIcon));

    SettingsPage * page;
    settings_page->setAlignment(Qt::AlignHCenter);
    settings_page->setWidget(page = new SettingsPage());
    connect(page, SIGNAL(proxyChanged(SettingsPage::ProxyType)), this, SLOT(proxyChanged(SettingsPage::ProxyType)));
    connect(page, SIGNAL(privacyChanged(SettingsPage::Privacy)), this, SLOT(privacyChanged(SettingsPage::Privacy)));
}

void MainWindow::on_actionShowDownloads_triggered()
{
    if (downloads_page)
    {
        ui->tabWidget->setCurrentWidget(downloads_page);
        return;
    }
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->insertTab(index, downloads_page = new DownloadsPage(this), "Downloads");
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, style()->standardIcon(QStyle::SP_DriveHDIcon));

    SettingsPage::downloadsHistoryModel()->select();
}

void MainWindow::proxyChanged(SettingsPage::ProxyType proxy_type)
{
    switch (proxy_type)
    {
    case SettingsPage::ProxyDisabled:
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        break;
    case SettingsPage::ProxySystem:
        foreach(QNetworkProxy proxy, QNetworkProxyFactory::systemProxyForQuery())
        {
            qDebug() << "proxy" << proxy.hostName() << proxy.port();
            QNetworkProxy::setApplicationProxy(proxy);
            break;
        }
        break;
    case SettingsPage::ProxySpecified:
        {
            QStringList list = SettingsPage::proxyAddress().split(":");
            if (list.count() < 2)
            {
                list << "8080";
            }
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, list.at(0), list.at(1).toInt()));
        }
        break;
    }
}

void MainWindow::openLastPages()
{
    foreach (QString url, SettingsPage::lastPages())
    {
        newWindowLink(QUrl(url));
    }
}

void MainWindow::saveLastPages()
{
    QStringList list;
    for (int i = 0; i < ui->tabWidget->count(); i++)
    {
        if (WebPage * webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(i)))
        {
            if (!webpage->isIncognito())
            {
                QString url = webpage->view()->url().toString();
                if (!url.isEmpty())
                {
                    list << url;
                }
            }
        }
    }
    SettingsPage::setLastPages(list);
}

void MainWindow::openHomePage()
{
    newWindowLink(QUrl(SettingsPage::homePage()));
}

void MainWindow::openEmptyPage()
{
    ui->actionAddTab->trigger();
}

void MainWindow::privacyChanged(SettingsPage::Privacy _privacy)
{
    add_tab->setDefaultAction(_privacy == SettingsPage::UsualPrivacy ? ui->actionAddTab : ui->actionAddIncognitoTab);
}

