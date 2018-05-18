#include "webpage.h"
#include "ui_webpage.h"
#include <QtWebKit/QWebSettings>
#include <QtWebKit/QWebHistory>
#include <QMenu>
#include <QtWebKitWidgets/QWebFrame>
#include <QFileInfo>
#include <QDir>
#include <QNetworkRequest>
#include <QDebug>
#include <QNetworkProxy>
#include <QImageReader>
#include <QBuffer>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QNetworkConfiguration>
#include <QInputDialog>
#include <QStandardPaths>
#include <QProgressDialog>
#include <QtWebKit/QWebElement>

#include "mainwindow.h"
#include "tabbar.h"
#include "utils.h"
#include "custom_classes.h"
#include "web_mime_types.h"
#include "downloadmanager.h"
#include "filemanager.h"

WebPage::WebPage(SettingsPage::Privacy _privacy, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebPage),
    privacy(_privacy),
    script("// sample script\r\ndocument.title = \"This is the new page title.\";"),
    tool_window(false),
    temporary(false),
    mw(0)
{
    ui->setupUi(this);

    view()->setPage(new CustomWebPage());

    page()->setNetworkAccessManager(new CustomNetworkAccessManager(this));

    connect(page(), SIGNAL(linkHovered(QString,QString,QString)), this, SIGNAL(linkHovered(QString,QString,QString)));
    connect(page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(initialLayoutCompleted()));
    connect(page(), SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadRequest(QNetworkRequest)));
    connect(page(), SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(unsupportedContent(QNetworkReply*)));
    connect(page(), SIGNAL(loadProgress(int)), ui->progressBar, SLOT(setValue(int)));
    connect(page(), SIGNAL(frameCreated(QWebFrame*)), this, SLOT(frameCreated(QWebFrame*)));

    inspector = new QWebInspector();
    inspector->setPage(page());
    ui->dockWidget->setWidget(inspector);
    ui->dockWidget->hide();

    connect(view(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
    connect(view()->pageAction(QWebPage::OpenLink), SIGNAL(triggered()), this, SLOT(actionOpenLink()));
    connect(view()->pageAction(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenLinkInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenFrameInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenFrameInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenImageInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenImageInNewWindow()));
    connect(view()->pageAction(QWebPage::DownloadLinkToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadLinkToDisk()));
    connect(view()->pageAction(QWebPage::DownloadImageToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadImageToDisk()));
    connect(view()->pageAction(QWebPage::InspectElement), SIGNAL(triggered()), this, SLOT(inspectCurrentElement()));

    ui->actionSearchImage->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    view()->pageAction(QWebPage::DownloadLinkToDisk)->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    view()->pageAction(QWebPage::DownloadImageToDisk)->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    view()->pageAction(QWebPage::InspectElement)->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));

    ui->actionLoadPage->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    ui->actionReloadPage->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));

    ui->loadPageButton->setDefaultAction(ui->actionLoadPage);
    ui->reloadButton->setDefaultAction(ui->actionReloadPage);
    ui->backButton->setDefaultAction(ui->actionBack);
    ui->forwardButton->setDefaultAction(ui->actionForward);

    completer = new UrlCompleter(this);
    completer->setModel(SettingsPage::completerHistoryModel());
    completer->setCompletionColumn(SettingsPage::completerHistoryModel()->fieldIndex("Phrase"));
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(10);

    QTableView * tv = new QTableView();
    tv->setGridStyle(Qt::NoPen);
    tv->setSelectionBehavior(QTableView::SelectRows);
    tv->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tv->horizontalHeader()->hide();
//    tv->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tv->verticalHeader()->setDefaultSectionSize(20);
    tv->verticalHeader()->hide();
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    completer->setPopup(tv);
    for (int i = 0; i < SettingsPage::completerHistoryModel()->columnCount(); i++)
    {
        if (i != SettingsPage::completerHistoryModel()->fieldIndex("Phrase") && i != SettingsPage::completerHistoryModel()->fieldIndex("Description"))
        {
            tv->hideColumn(i);
        }
    }
    tv->setItemDelegateForColumn(SettingsPage::completerHistoryModel()->fieldIndex("Description"), new CompleterColumnDelegate());
    ui->webAddress->setCompleter(completer);

    if (privacy == SettingsPage::IncognitoPrivacy)
    {
        setIncognito();
    }
    else
    {
        setWindowIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    }

    dmanager = new DownloadManager();
    connect(dmanager, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updownProgress(qint64,qint64)));
    connect(dmanager, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
    connect(dmanager, SIGNAL(downloadFinished(QNetworkReply *)), this, SLOT(downloadFinished(QNetworkReply *)));
    dmanager->setCookieJar(new SharedCookieJar((SettingsPage::Privacy)privacy));
    page()->networkAccessManager()->setCookieJar(new SharedCookieJar((SettingsPage::Privacy)privacy));

    fmanager = new FileManager(this);
    fmanager->setWebPage(this);
    connect(fmanager, SIGNAL(showNotifier(QString,int)), this, SIGNAL(showNotifier(QString,int)));
}

WebPage::~WebPage()
{
//    qDebug() << "dtor webpage";
    if (dmanager->currentReply() && dmanager->currentReply()->isRunning())
    {
        connect(dmanager, SIGNAL(downloadFinished(QNetworkReply*)), dmanager, SLOT(deleteLater()), Qt::QueuedConnection);
    }
    dmanager->setCookieJar(new QNetworkCookieJar()); // force prev jar dtor calling
    page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar()); // force prev jar dtor calling
    delete ui;
}

void WebPage::setMainWindow(const MainWindow * _mw)
{
    if (CustomWebPage * custom_page = dynamic_cast<CustomWebPage *>(page()))
    {
        custom_page->setMainWindow(_mw);
    }
    mw = (MainWindow *)_mw;
}

QWebView * WebPage::view() const
{
    return ui->webView;
}

QWebPage * WebPage::page() const
{
    return view() ? view()->page() : 0;
}

void WebPage::load(const QUrl & url)
{
//    qDebug() << "load" << url;

    Meow::waitEventsProcessing(); // hack
    view()->load(CustomNetworkRequest(url));
}

void WebPage::on_actionLoadPage_triggered()
{
    QUrl url(ui->webAddress->text());
    QUrl url_copy(url);
    if (url.toString().trimmed().compare("about:blank", Qt::CaseInsensitive) != 0)
    {
        if (!url.toString().contains("."))
        {
            QString search_engine = SettingsPage::currentSearchEngineUrl().arg(QString(url_copy.toEncoded()));
            if (!search_engine.isEmpty())
            {
                updateCompleter(url_copy.toString(QUrl::RemoveScheme | QUrl::RemovePort), "");
                url = search_engine;
            }
            else
            {
                url = "0.0.0.0";
            }
        }
        if ((privacy == SettingsPage::IncognitoPrivacy && url.scheme().compare("http", Qt::CaseInsensitive) == 0) || url.scheme().isEmpty())
        {
            url.setScheme("https");
        }
    }
    load(current_url = url);
}

void WebPage::on_actionReloadPage_triggered()
{
    if (view()->url() == current_url)
    {
        view()->triggerPageAction(QWebPage::ReloadAndBypassCache);
    }
    else
    {
        load(current_url);
    }
}

void WebPage::on_actionBack_triggered()
{
    if (view()->history()->canGoBack())
    {
        current_url = view()->history()->backItem().url();
        view()->back();
        applyIcon();
    }
}

void WebPage::on_actionForward_triggered()
{
    if (view()->history()->canGoForward())
    {
        current_url = view()->history()->forwardItem().url();
        view()->forward();
        applyIcon();
    }
}

void WebPage::drawMeowCat()
{
    if (current_url.toString().trimmed().compare("about:blank", Qt::CaseInsensitive) == 0) // draw cat
    {
        QStringList div_style = QStringList() << "position: absolute" << "top: 0" << "bottom: 0" << "left: 0" << "right: 0" << "margin: auto" << "width: 30%" << "height: 30%";
        if (privacy == SettingsPage::UsualPrivacy)
        {
            div_style << "opacity: 0.1";
        }
        page()->mainFrame()->setHtml(
                    "<html><body><figure><div id=\"meow_cat\" style=\"" +
                    div_style.join("; ") + "\">" +
                    Meow::resourceString(":/Images/robocat.svg") +
                    "</div></figure><script type=\"text/javascript\">var meow_cat=document.getElementById(\"meow_cat\");</script></body></html>");
    }
}

void WebPage::on_webView_loadFinished(bool status)
{
    if (status)
    {
        if (temporary)
        {
            if (requested_url != view()->url())
            {
                downloadRequest(CustomNetworkRequest(view()->url()));
            }
        }
        else
        {
            drawMeowCat();
            runUserScriptRecursive(page()->mainFrame());
        }
    }
    else
    {
        qDebug() << "loading failed!!!";
//        page()->mainFrame()->setHtml("<html><body><div style=\"position: absolute; top: 0; bottom: 0; left: 0; right: 0; margin: auto; width: 200px; height: 100px;\"><h1>Loading failed</h1></div></body></html>", view()->url());
    }
    applyIcon();
    ui->progressBar->setValue(0);
    ui->dockWidget->setWindowTitle("Web Inspector - " + view()->url().toString());
}

void WebPage::on_webView_titleChanged(const QString &)
{
    applyTitle();
}

void WebPage::on_webView_urlChanged(const QUrl & url)
{
    current_url = url;
    ui->webAddress->setText(url.toString());
}

void WebPage::on_webView_iconChanged()
{
    applyIcon();
}

void WebPage::showEvent(QShowEvent *)
{
    applyTitle();
    ui->webAddress->setFocus();
}

void WebPage::on_webView_loadStarted()
{
    applyIcon();
}

void WebPage::applyIcon()
{
    if (privacy == SettingsPage::IncognitoPrivacy)
    {
        return;
    }
    if (QTabWidget * tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        const int index = tw->indexOf(this);
        QIcon icon = QWebSettings::iconForUrl(current_url);
        tw->setTabIcon(index, icon.isNull() ? windowIcon() : icon);
    }
}

void WebPage::applyTitle()
{
    QString title = current_page_title = view()->title();
    if (view()->url().isEmpty() || view()->url().toString().trimmed().compare("about:blank", Qt::CaseInsensitive) == 0)
    {
        title = "New Tab";
        ui->webAddress->setText("");
    }
    if (QTabWidget * tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        if (TabBar * tb = qobject_cast<TabBar *>(tw->tabBar()))
        {
            const int index = tw->indexOf(this);
            tw->setTabToolTip(index, title);
            tw->setTabText(index, title);
        }
    }
}

void WebPage::runScript()
{
    bool ok = false;
    QString new_script;
    page()->mainFrame()->evaluateJavaScript(new_script = QInputDialog::getMultiLineText(topLevelWidget(), "Run Javascript code", "Javascript code", script, & ok));
    if (ok)
    {
        script = new_script;
    }
}

void WebPage::customContextMenuRequested(const QPoint & pos)
{
    QMenu * menu = page()->createStandardContextMenu();
    if (menu->actions().contains(view()->pageAction(QWebPage::OpenImageInNewWindow)))
    {
        menu->insertAction(view()->pageAction(QWebPage::OpenImageInNewWindow), ui->actionSearchImage);
    }
    if (!page()->selectedText().isEmpty())
    {
        menu->insertAction(menu->actions().first(), ui->actionTranslateSelected);
    }
    menu->exec(mapToGlobal(current_pos = pos));
    menu->deleteLater();
}

QWebHitTestResult WebPage::hitTest(QPoint pos) const
{
    return view()->page()->mainFrame()->hitTestContent(pos);
}

void WebPage::actionOpenLink()
{
    QWebHitTestResult hit_test = hitTest(current_pos);
    QUrl url = hit_test.linkUrl();
    if (!url.isEmpty() && url.isValid())
    {
        qDebug() << "actionOpenLink" << url;
        if (mw && mw->lastPage())
        {
            mw->lastPage()->load(url);
        }
        else
        {
            load(url);
        }
    }
}

void WebPage::actionOpenLinkInNewWindow()
{
    QUrl url = hitTest(current_pos).linkUrl();
    if (!url.isEmpty() && url.isValid())
    {
        emit newWindowLink(url);
    }
}

void WebPage::actionOpenFrameInNewWindow()
{
    QWebFrame * frame = hitTest(current_pos).frame();
    if (!frame->requestedUrl().isEmpty() && frame->requestedUrl().isValid())
    {
        createToolWindow(frame->requestedUrl());
//        emit newWindowLink(frame->requestedUrl());
    }
}

void WebPage::actionOpenImageInNewWindow()
{
    QUrl url = hitTest(current_pos).imageUrl();
    if (!url.isEmpty() && url.isValid())
    {
        emit newWindowLink(url);
    }
}

void WebPage::actionDownloadLinkToDisk()
{
    QWebHitTestResult res = hitTest(current_pos);
    current_alt_text = res.element().toPlainText();
}

void WebPage::actionDownloadImageToDisk()
{
    QWebHitTestResult res = hitTest(current_pos);
    current_alt_text = res.element().attribute("alt");
}

WebPage * WebPage::createHiddenWindow(QUrl url, Qt::ConnectionType conn) const
{
    qDebug() << "creating hidden page";
    WebPage * redirect_page = new WebPage(privacy, const_cast<WebPage *>(this));
    redirect_page->hide();
    connect(redirect_page, SIGNAL(downloadFinished()), this, SIGNAL(downloadFinished()));
    connect(redirect_page, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updownProgress(qint64,qint64)));
    redirect_page->fmanager->setStateFrom(fmanager);
    redirect_page->requested_url = url;
    redirect_page->temporary = true;
    if (!url.isEmpty())
    {
        QMetaObject::invokeMethod(redirect_page, "load", conn, Q_ARG(QUrl, url));
    }
    return redirect_page;
}

void WebPage::downloadFinished(QNetworkReply * reply)
{
    qDebug() << "downloadFinished" << reply;
    fmanager->saveReplyToDisk(reply);

    ui->progressBar->setValue(0);
    emit downloadFinished();

    if (temporary)
    {
        qDebug() << "deleting hidden page";
        deleteLater();
    }
}

void WebPage::redirected(const QUrl & url)
{
    qDebug() << "redirected" << url;
}

void WebPage::updateCompleter(QString phrase, QString desc)
{
    if (privacy == SettingsPage::UsualPrivacy)
    {
        if (!phrase.isEmpty())
        {
            if (phrase.startsWith("//"))
            {
                phrase = phrase.mid(2);
            }
            if (desc.startsWith("//"))
            {
                desc = desc.mid(2);
            }
            SettingsPage::addCompleterPhrase(phrase, desc);
        }
    }
}

void WebPage::initialLayoutCompleted()
{
    if (view()->url().host() != QUrl(SettingsPage::currentSearchEngineUrl()).host())
    {
        if (view()->title().isEmpty())
        {
            updateCompleter(view()->url().toString(QUrl::RemoveScheme | QUrl::RemovePort), QString());
        }
        else
        {
            updateCompleter(view()->title(), view()->url().toString(QUrl::RemoveScheme | QUrl::RemovePort));
        }
    }
    ui->dockWidget->setWindowTitle("Web Inspector - " + view()->url().toString());
    runUserScriptRecursive(page()->mainFrame());
}

void WebPage::runUserScriptRecursive(QWebFrame * frame)
{
    frame->evaluateJavaScript(SettingsPage::userScript());
    foreach (QWebFrame * child_frame, frame->childFrames())
    {
        runUserScriptRecursive(child_frame);
    }
}

void WebPage::frameCreated(QWebFrame * frame)
{
}

void WebPage::downloadRequest(const QNetworkRequest & request)
{
    dmanager->downloadRequest(request);
}

void WebPage::unsupportedContent(QNetworkReply * reply)
{
    qDebug() << "unsupported content";
    downloadRequest(CustomNetworkRequest(reply->url()));
    delete reply;
}

void WebPage::featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature)
{
    qDebug() << frame->baseUrl() << "requested" << feature;
    page()->setFeaturePermission(frame, feature, QWebPage::PermissionDeniedByUser);
}

void WebPage::setIncognito()
{
    setWindowIcon(style()->standardIcon(QStyle::SP_VistaShield));

    QWebSettings * settings = view()->settings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, false);
    settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
    settings->setThirdPartyCookiePolicy(QWebSettings::AlwaysBlockThirdPartyCookies);
}

SettingsPage::Privacy WebPage::privacyMode() const
{
    return privacy;
}

bool WebPage::isToolWindow() const
{
    return tool_window;
}

bool WebPage::isHiddenWindow() const
{
    return temporary;
}

WebPage * WebPage::createToolWindow(QUrl url, QString title, Qt::ConnectionType conn) const
{
    WebPage * new_webpage = new WebPage(privacy, const_cast<WebPage *>(this));
    new_webpage->setWindowTitle(title);
    new_webpage->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    for (int i = 0; i < new_webpage->ui->horizontalLayout->count(); i++)
    {
        QLayoutItem * item = new_webpage->ui->horizontalLayout->itemAt(i);
        if (item->widget())
        {
            item->widget()->hide();
        }
    }
    new_webpage->ui->progressBar->hide();
    new_webpage->show();
    new_webpage->activateWindow();
    new_webpage->tool_window = true;
    if (!url.isEmpty())
    {
        QMetaObject::invokeMethod(new_webpage, "load", conn, Q_ARG(QUrl, url));
    }
    return new_webpage;
}

void WebPage::updownProgress(qint64 processed, qint64 total)
{
    if (total > 0)
    {
        ui->progressBar->setValue(ui->progressBar->maximum() * processed / total);
    }
}

void WebPage::inspectCurrentElement()
{
    ui->dockWidget->show();
}


void WebPage::on_actionSearchImage_triggered()
{
    QUrl url = hitTest(current_pos).imageUrl();
    if (!url.isEmpty() && url.isValid())
    {
        QString image_search_url = SettingsPage::currentImageSearchEngineUrl().arg(url.toString());
        qDebug() << "search image url" << image_search_url;
        emit newWindowLink(QUrl(image_search_url));
    }
}

void WebPage::savePage()
{
    fmanager->savePage();
}

void WebPage::on_actionTranslateSelected_triggered()
{
    QString text;
    if (ui->webAddress->hasFocus())
    {
        text = ui->webAddress->selectedText();
    }
    else
    if (!page()->selectedText().isEmpty())
    {
        text = page()->selectedText();
    }
    if (!text.isEmpty())
    {
        QString translate_url = SettingsPage::currentTranslateEngineUrl().arg(text);
        qDebug() << "translate url" << translate_url;
        emit newWindowLink(QUrl(translate_url));
    }
}

void WebPage::on_webAddress_customContextMenuRequested(const QPoint &pos)
{
    QMenu * menu = ui->webAddress->createStandardContextMenu();
    if (!ui->webAddress->selectedText().isEmpty())
    {
        menu->insertSeparator(menu->actions().front());
        menu->insertAction(menu->actions().front(), ui->actionTranslateSelected);
    }
    menu->exec(mapToGlobal(pos));
}

bool WebPage::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

