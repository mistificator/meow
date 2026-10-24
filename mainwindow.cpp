#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tabbar.h"

#include <QToolButton>
#include <QLabel>
#include <QDir>
#include <QApplication>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QFontMetrics>

#include "custom_classes.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    last_page(nullptr),
    settings_page(nullptr),
    downloads_page(nullptr)
{
    proxyChanged(SettingsPage::proxyType());

    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(conf);

    ui->setupUi(this);

    // Устанавливаем кастомный TabBar
    bool movable = ui->tabWidget->isMovable();
    bool closable = ui->tabWidget->tabsClosable();
    TabWidgetOpener::setTabBar(ui->tabWidget, new TabBar(ui->tabWidget));
    ui->tabWidget->setMovable(movable);
    ui->tabWidget->setTabsClosable(closable);

    ui->mainToolBar->hide();

    notifier = new QLabel(this);
    notifier->hide();
    notifier_timer = new QTimer(this);
    notifier_timer->setSingleShot(true);
    connect(notifier_timer, &QTimer::timeout, this, &MainWindow::hideNotifier);

    setWindowIcon(QIcon(":/Images/16x16.png"));

    ui->actionAddTab->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    ui->actionAddIncognitoTab->setIcon(style()->standardIcon(QStyle::SP_VistaShield));
    ui->actionShowSettings->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    ui->actionShowDownloads->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    newtab_menu.addAction(ui->actionAddTab);
    newtab_menu.addAction(ui->actionAddIncognitoTab);
    newtab_menu.addSeparator();
    newtab_menu.addAction(ui->actionShowSettings);
    newtab_menu.addAction(ui->actionShowDownloads);

    // Фиксированная кнопка справа от панели вкладок (не является вкладкой TabWidget)
    add_tab = new QToolButton(ui->tabWidget);
    add_tab->setToolButtonStyle(Qt::ToolButtonIconOnly);
    add_tab->setPopupMode(QToolButton::MenuButtonPopup);
    add_tab->setAutoRaise(true);
    add_tab->setMinimumSize(36, 26);
    add_tab->setDefaultAction(SettingsPage::privacyPolicy() == SettingsPage::UsualPrivacy
                                  ? ui->actionAddTab
                                  : ui->actionAddIncognitoTab);
    add_tab->setMenu(&newtab_menu);
    ui->tabWidget->setCornerWidget(add_tab, Qt::TopRightCorner);

    ui->tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabWidget->tabBar(), &QWidget::customContextMenuRequested,
            this, &MainWindow::tabCustomContextMenuRequested);

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

    last_page = nullptr;
}

MainWindow::~MainWindow()
{
    saveLastPages();
    delete ui;
}

void MainWindow::on_actionAddTab_triggered()
{
    last_page = new WebPage(SettingsPage::UsualPrivacy, this);
    int index = ui->tabWidget->addTab(last_page, QString());
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, last_page->windowIcon());
    last_page->setMainWindow(this);
    last_page->load(QUrl("about:blank"));
    connect(last_page, &WebPage::newWindowLink, this, &MainWindow::newWindowLink);
    connect(last_page, &WebPage::linkHovered, this, &MainWindow::linkHovered);
    connect(last_page, &WebPage::showNotifier, this, &MainWindow::showNotifier);
}

void MainWindow::on_actionAddIncognitoTab_triggered()
{
    last_page = new WebPage(SettingsPage::IncognitoPrivacy, this);
    int index = ui->tabWidget->addTab(last_page, QString());
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, last_page->windowIcon());
    last_page->setMainWindow(this);
    last_page->load(QUrl("about:blank"));
    connect(last_page, &WebPage::newWindowLink, this, &MainWindow::newWindowLink);
    connect(last_page, &WebPage::linkHovered, this, &MainWindow::linkHovered);
    connect(last_page, &WebPage::showNotifier, this, &MainWindow::showNotifier);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;

    showNotifier(ui->tabWidget->tabText(index).trimmed() + " closed");
    QWidget *w = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    if (WebPage *page = dynamic_cast<WebPage *>(w))
    {
        page->load(QUrl("about:blank"));
    }
    if (w == settings_page)
    {
        settings_page = nullptr;
    }
    if (w == downloads_page)
    {
        downloads_page = nullptr;
    }
    w->deleteLater();
    last_page = nullptr;

    // Если закрыта последняя вкладка, создаем новую пустую, чтобы панель не оставалась пустой
    if (ui->tabWidget->count() == 0)
    {
        openEmptyPage();
    }
}

void MainWindow::newWindow(SettingsPage::Privacy p)
{
    switch (p)
    {
    case SettingsPage::UsualPrivacy:
        ui->actionAddTab->trigger();
        break;
    case SettingsPage::IncognitoPrivacy:
        ui->actionAddIncognitoTab->trigger();
        break;
    }
}

void MainWindow::newWindowLink(QUrl url)
{
    if (last_page == nullptr)
    {
        SettingsPage::Privacy privacy = SettingsPage::UsualPrivacy;
        if (WebPage *webpage = dynamic_cast<WebPage *>(sender()))
        {
            privacy = webpage->privacyMode();
        }
        newWindow(privacy);
    }
    last_page->load(url);
    last_page = nullptr;
}

void MainWindow::linkHovered(const QString &link, const QString &, const QString &)
{
    ui->statusBar->showMessage(link, 10000);
}

void MainWindow::showNotifier(QString message, int timeout)
{
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(notifier);
    notifier->setGraphicsEffect(effect);
    notifier->setStyleSheet("border: 3px solid gray;border-radius:20px;background-color:#ffffff;color:gray");
    notifier->setAlignment(Qt::AlignCenter);
    notifier->setText(message);
    notifier->resize(QFontMetrics(notifier->font()).horizontalAdvance(message) + 20, notifier->height());
    QPoint pt = QPoint(ui->centralWidget->rect().right(), ui->centralWidget->rect().bottom());
    notifier->move(pt.x() - notifier->width(), pt.y() - notifier->height());

    QPropertyAnimation *a = new QPropertyAnimation(effect, "opacity");
    a->setDuration(1000);
    a->setStartValue(0);
    a->setEndValue(1);
    a->setEasingCurve(QEasingCurve::InBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    notifier->show();
    notifier_timer->start(timeout);
}

void MainWindow::hideNotifier()
{
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(notifier);
    notifier->setGraphicsEffect(effect);
    QPropertyAnimation *a = new QPropertyAnimation(effect, "opacity");
    a->setDuration(1000);
    a->setStartValue(1);
    a->setEndValue(0);
    a->setEasingCurve(QEasingCurve::OutBack);
    a->start(QPropertyAnimation::DeleteWhenStopped);
    connect(a, &QPropertyAnimation::finished, notifier, &QWidget::hide);
    connect(a, &QPropertyAnimation::finished, notifier_timer, &QTimer::stop);
}

void MainWindow::on_actionShowSettings_triggered()
{
    if (settings_page)
    {
        ui->tabWidget->setCurrentWidget(settings_page);
        return;
    }
    settings_page = new QScrollArea(this);
    int index = ui->tabWidget->addTab(settings_page, "Settings");
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, style()->standardIcon(QStyle::SP_FileDialogListView));

    SettingsPage *page = new SettingsPage();
    settings_page->setAlignment(Qt::AlignHCenter);
    settings_page->setWidget(page);
    connect(page, &SettingsPage::proxyChanged, this, &MainWindow::proxyChanged);
    connect(page, &SettingsPage::privacyChanged, this, &MainWindow::privacyChanged);
    connect(page, &SettingsPage::showNotifier, this, &MainWindow::showNotifier);
}

void MainWindow::on_actionShowDownloads_triggered()
{
    if (downloads_page)
    {
        ui->tabWidget->setCurrentWidget(downloads_page);
        return;
    }
    downloads_page = new DownloadsPage(this);
    int index = ui->tabWidget->addTab(downloads_page, "Downloads");
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    SettingsPage::downloadsHistoryModel()->select();
    connect(downloads_page, &DownloadsPage::newWindowLink, this, &MainWindow::newWindowLink);
}

void MainWindow::proxyChanged(SettingsPage::ProxyType proxy_type)
{
    switch (proxy_type)
    {
    case SettingsPage::ProxyDisabled:
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        break;
    case SettingsPage::ProxySystem:
        for (const QNetworkProxy &proxy : QNetworkProxyFactory::systemProxyForQuery())
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
    const QStringList pages = SettingsPage::lastPages();
    if (pages.isEmpty())
    {
        openEmptyPage();
        return;
    }
    for (const QString &url : pages)
    {
        newWindowLink(QUrl(url));
    }
}

void MainWindow::saveLastPages()
{
    QStringList list;
    for (int i = 0; i < ui->tabWidget->count(); i++)
    {
        if (WebPage *webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(i)))
        {
            if (webpage->privacyMode() == SettingsPage::UsualPrivacy)
            {
                QString url = webpage->currentUrl().toString();
                if (!url.isEmpty() && !url.startsWith("data:text/html", Qt::CaseInsensitive))
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
    add_tab->setDefaultAction(_privacy == SettingsPage::UsualPrivacy
                                  ? ui->actionAddTab
                                  : ui->actionAddIncognitoTab);
}

void MainWindow::tabCustomContextMenuRequested(const QPoint &pos)
{
    int tabIndex = ui->tabWidget->tabBar()->tabAt(pos);
    if (tabIndex < 0)
        return;

    ui->tabWidget->setCurrentIndex(tabIndex);
    QMenu menu;
    if (dynamic_cast<WebPage *>(ui->tabWidget->widget(tabIndex)))
    {
        menu.addAction(ui->actionCloseAllOtherTabs);
        menu.addSeparator();
        menu.addAction(ui->actionSavePage);
        menu.addAction(ui->actionDownloadPage);
        menu.addAction(ui->actionInspectPage);
        menu.addSeparator();
        menu.addAction(ui->actionRunScript);
    }
    if (menu.actions().isEmpty())
    {
        return;
    }
    menu.exec(ui->tabWidget->tabBar()->mapToGlobal(pos));
}

void MainWindow::on_actionSavePage_triggered()
{
    if (WebPage *webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(ui->tabWidget->currentIndex())))
    {
        webpage->savePage();
    }
}

void MainWindow::on_actionDownloadPage_triggered()
{
    if (WebPage *webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(ui->tabWidget->currentIndex())))
    {
        CustomNetworkRequest::Headers h;
        h.header = "Content-Type";
        h.value = "text/html";
        webpage->downloadRequest(CustomNetworkRequest(webpage->view()->url(), QList<CustomNetworkRequest::Headers>() << h));
    }
}

void MainWindow::on_actionRunScript_triggered()
{
    if (WebPage *webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(ui->tabWidget->currentIndex())))
    {
        webpage->runScript();
    }
}

void MainWindow::on_actionInspectPage_triggered()
{
    if (WebPage *webpage = dynamic_cast<WebPage *>(ui->tabWidget->widget(ui->tabWidget->currentIndex())))
    {
        webpage->inspectCurrentElement();
    }
}

void MainWindow::on_actionCloseAllOtherTabs_triggered()
{
    setUpdatesEnabled(false);
    QWidget *current = ui->tabWidget->currentWidget();
    for (int i = ui->tabWidget->count() - 1; i >= 0; --i)
    {
        if (ui->tabWidget->widget(i) != current)
        {
            on_tabWidget_tabCloseRequested(i);
        }
    }
    setUpdatesEnabled(true);
}
