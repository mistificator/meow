#include "mainwindow.h"
#include "ui_mainwindow.h"
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

class TabWidgetOpener: public QTabWidget
{
public:
    static QTabBar * tabBar(QTabWidget * tw)
    {
        return ((TabWidgetOpener *)tw)->QTabWidget::tabBar();
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    last_page(0)
{
    foreach(QNetworkProxy proxy, QNetworkProxyFactory::systemProxyForQuery())
    {
        qDebug() << "proxy" << proxy.hostName() << proxy.port();
        QNetworkProxy::setApplicationProxy(proxy);
        break;
    }

    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

    ui->setupUi(this);
    ui->mainToolBar->hide();

    notifier = new QLabel(this);
    notifier->hide();
    notifier_timer = new QTimer(this);
    notifier_timer->setSingleShot(true);

    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    ui->actionAddTab->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));

    newtab_menu.addAction(ui->actionAddTab);

    // taken from https://stackoverflow.com/questions/19975137/how-can-i-add-a-new-tab-button-next-to-the-tabs-of-a-qmdiarea-in-tabbed-view-m/27127109#27127109
    QToolButton *tb = new QToolButton();
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setPopupMode(QToolButton::MenuButtonPopup);
    tb->setDefaultAction(ui->actionAddTab);
    tb->setMenu(& newtab_menu);
    // Add empty, not enabled tab to tabWidget
    ui->tabWidget->addTab(new QWidget(), QString());
    ui->tabWidget->setTabEnabled(0, false);
    // Add tab button to current tab. Button will be enabled, but tab -- not
    TabWidgetOpener::tabBar(ui->tabWidget)->setTabButton(0, QTabBar::RightSide, tb);

    temp_dir = QDir::tempPath() + "/meow";
    QDir().mkpath(temp_dir);
    QWebSettings::enablePersistentStorage(temp_dir);    
    qDebug() << "temp dir" << temp_dir;

    ui->actionAddTab->trigger();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAddTab_triggered()
{
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->insertTab(index, last_page = new WebPage(this), QString());
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->setTabIcon(index, style()->standardIcon(QStyle::SP_ComputerIcon));

    connect(last_page, SIGNAL(newWindowLink(QUrl)), this, SLOT(newWindowLink(QUrl)));
    connect(last_page, SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
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
    w->deleteLater();

    ui->tabWidget->setCurrentIndex(qMax(index - 1, 0));
}

void MainWindow::newWindowLink(QUrl url)
{
    ui->actionAddTab->trigger();
    last_page->view()->load(url);
}

void MainWindow::linkHovered(const QString & link, const QString & title, const QString & textContent)
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
