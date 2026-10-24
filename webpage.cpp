#include "webpage.h"
#include "ui_webpage.h"

#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QNetworkRequest>
#include <QDebug>
#include <QTableView>
#include <QHeaderView>
#include <QInputDialog>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>

#include "mainwindow.h"
#include "utils.h"
#include "custom_classes.h"
#include "downloadmanager.h"
#include "filemanager.h"

static bool isBlankUrl(const QUrl &url)
{
    const QString s = url.toString().trimmed();
    return s.isEmpty() ||
           s.compare("about:blank", Qt::CaseInsensitive) == 0 ||
           s.startsWith("data:text/html", Qt::CaseInsensitive);
}

QWebEngineProfile *WebPage::sharedProfile(SettingsPage::Privacy p)
{
    static QWebEngineProfile *profiles[2] = {nullptr, nullptr};
    if (!profiles[p])
    {
        if (p == SettingsPage::UsualPrivacy)
        {
            profiles[p] = new QWebEngineProfile("MeowDefault", qApp);
            profiles[p]->setPersistentStoragePath(SettingsPage::tempDir());
            profiles[p]->setCachePath(SettingsPage::tempDir());
        }
        else
        {
            profiles[p] = new QWebEngineProfile(qApp); // Off-the-record (Incognito)
        }

        // Синхронизируем куки профиля с SharedCookieJar для DownloadManager
        SharedCookieJar *syncJar = new SharedCookieJar(p, profiles[p]);
        for (const QNetworkCookie &c : SharedCookieJar::getAllSharedCookies(p))
        {
            profiles[p]->cookieStore()->setCookie(c);
        }
        QObject::connect(profiles[p]->cookieStore(), &QWebEngineCookieStore::cookieAdded,
                         profiles[p], [syncJar](const QNetworkCookie &cookie) {
                             syncJar->insertCookie(cookie);
                         });
        QObject::connect(profiles[p]->cookieStore(), &QWebEngineCookieStore::cookieRemoved,
                         profiles[p], [syncJar](const QNetworkCookie &cookie) {
                             syncJar->deleteCookie(cookie);
                         });
        profiles[p]->cookieStore()->loadAllCookies();
    }
    return profiles[p];
}

WebPage::WebPage(SettingsPage::Privacy _privacy, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebPage),
    privacy(_privacy),
    script("// sample script\r\ndocument.title = \"This is the new page title.\";"),
    inspector_view(nullptr),
    tool_window(false),
    temporary(false),
    mw(nullptr)
{
    ui->setupUi(this);

    QWebEngineProfile *prof = sharedProfile(privacy);
    CustomWebPage *custom_page = new CustomWebPage(prof, this);
    custom_page->setWebPageWidget(this);
    view()->setPage(custom_page);

    connect(page(), &QWebEnginePage::linkHovered, this, [this](const QString &url) {
        emit linkHovered(url, QString(), QString());
    });
    connect(page(), &QWebEnginePage::loadProgress, ui->progressBar, &QProgressBar::setValue);
    connect(page(), &QWebEnginePage::featurePermissionRequested, this, &WebPage::featurePermissionRequested);
    connect(prof, &QWebEngineProfile::downloadRequested, this, &WebPage::handleProfileDownload, Qt::UniqueConnection);

    // Инспектор (DevTools) для Qt6 WebEngine
    inspector_view = new QWebEngineView(ui->dockWidget);
    ui->dockWidget->setWidget(inspector_view);
    ui->dockWidget->hide();

    connect(view(), &QWidget::customContextMenuRequested, this, &WebPage::customContextMenuRequested);
    connect(view()->pageAction(QWebEnginePage::OpenLinkInThisWindow), &QAction::triggered, this, &WebPage::actionOpenLink);
    connect(view()->pageAction(QWebEnginePage::OpenLinkInNewWindow), &QAction::triggered, this, &WebPage::actionOpenLinkInNewWindow);
    connect(view()->pageAction(QWebEnginePage::OpenLinkInNewTab), &QAction::triggered, this, &WebPage::actionOpenLinkInNewWindow);
    connect(view()->pageAction(QWebEnginePage::DownloadLinkToDisk), &QAction::triggered, this, &WebPage::actionDownloadLinkToDisk);
    connect(view()->pageAction(QWebEnginePage::DownloadImageToDisk), &QAction::triggered, this, &WebPage::actionDownloadImageToDisk);
    connect(view()->pageAction(QWebEnginePage::InspectElement), &QAction::triggered, this, &WebPage::inspectCurrentElement);

    ui->actionSearchImage->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    view()->pageAction(QWebEnginePage::DownloadLinkToDisk)->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    view()->pageAction(QWebEnginePage::DownloadImageToDisk)->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    view()->pageAction(QWebEnginePage::InspectElement)->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));

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

    QTableView *tv = new QTableView();
    tv->setGridStyle(Qt::NoPen);
    tv->setSelectionBehavior(QTableView::SelectRows);
    tv->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tv->horizontalHeader()->hide();
    tv->verticalHeader()->setDefaultSectionSize(20);
    tv->verticalHeader()->hide();
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    completer->setPopup(tv);
    for (int i = 0; i < SettingsPage::completerHistoryModel()->columnCount(); i++)
    {
        if (i != SettingsPage::completerHistoryModel()->fieldIndex("Phrase") &&
            i != SettingsPage::completerHistoryModel()->fieldIndex("Description"))
        {
            tv->hideColumn(i);
        }
    }
    tv->setItemDelegateForColumn(SettingsPage::completerHistoryModel()->fieldIndex("Description"), new CompleterColumnDelegate(tv));
    ui->webAddress->setCompleter(completer);

    if (privacy == SettingsPage::IncognitoPrivacy)
    {
        setIncognito();
    }
    else
    {
        setWindowIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    }

    // Исправлена утечка памяти: передаем this родителем либо корректно управляем жизнью dmanager
    dmanager = new DownloadManager(this);
    connect(dmanager, &DownloadManager::downloadProgress, this, &WebPage::updownProgress);
    connect(dmanager, &DownloadManager::downloadProgress, this, &WebPage::downloadProgress);
    connect(dmanager, &DownloadManager::downloadFinished, this, &WebPage::networkDownloadFinished);
    cookie_jar = new SharedCookieJar(privacy, dmanager);
    dmanager->setCookieJar(cookie_jar);

    fmanager = new FileManager(this);
    fmanager->setWebPage(this);
    connect(fmanager, &FileManager::showNotifier, this, &WebPage::showNotifier);
}

WebPage::~WebPage()
{
    if (inspector_view && inspector_view->page())
    {
        page()->setDevToolsPage(nullptr);
    }
    delete ui;
}

void WebPage::setMainWindow(const MainWindow *_mw)
{
    if (CustomWebPage *custom_page = dynamic_cast<CustomWebPage *>(page()))
    {
        custom_page->setMainWindow(_mw);
    }
    mw = const_cast<MainWindow *>(_mw);
}

QWebEngineView *WebPage::view() const
{
    return ui->webView;
}

QWebEnginePage *WebPage::page() const
{
    return view() ? view()->page() : nullptr;
}

QString WebPage::fetchHtmlSync() const
{
    if (!page())
        return QString();
    QString result;
    QEventLoop loop;
    page()->toHtml([&result, &loop](const QString &html) {
        result = html;
        loop.quit();
    });
    loop.exec();
    return result;
}

QVariant WebPage::evalJsSync(const QString &jsSource) const
{
    if (!page())
        return QVariant();
    QVariant result;
    QEventLoop loop;
    page()->runJavaScript(jsSource, [&result, &loop](const QVariant &v) {
        result = v;
        loop.quit();
    });
    loop.exec();
    return result;
}

void WebPage::load(const QUrl &url)
{
    Meow::waitEventsProcessing();
    view()->load(url);
}

void WebPage::on_actionLoadPage_triggered()
{
    QUrl url(ui->webAddress->text());
    QUrl url_copy(url);
    if (url.toString().trimmed().compare("about:blank", Qt::CaseInsensitive) != 0)
    {
        if (!url.toString().contains("."))
        {
            QString search_engine = SettingsPage::currentSearchEngineUrl().arg(QString::fromUtf8(url_copy.toEncoded()));
            if (!search_engine.isEmpty())
            {
                updateCompleter(url_copy.toString(QUrl::RemoveScheme | QUrl::RemovePort), "");
                url = QUrl(search_engine);
            }
            else
            {
                url = QUrl("https://0.0.0.0");
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
        view()->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);
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
    if (isBlankUrl(current_url))
    {
        const QString opacity = (privacy == SettingsPage::UsualPrivacy) ? "0.1" : "1.0";
        const QString svg = Meow::resourceString(":/Images/robocat.svg");

        const QString html = QString(
                                 "<style>"
                                 "html, body { margin: 0; padding: 0; width: 100%; height: 100%; overflow: hidden; }"
                                 "#meow_cat { position: fixed; inset: 0; margin: auto; width: 30vmin; height: 30vmin; "
                                 "            opacity: %1; display: flex; align-items: center; justify-content: center; pointer-events: none; }"
                                 "#meow_cat svg { width: 100%; height: 100%; display: block; }"
                                 "</style>"
                                 "<div id=\"meow_cat\">%2</div>"
                                 ).arg(opacity, svg);

        // Безопасно передаем HTML в JS через JSON-строку, не меняя URL страницы с about:blank
        const QString jsonArg = QString::fromUtf8(QJsonDocument(QJsonArray{html}).toJson(QJsonDocument::Compact));
        const QString js = QString(
                               "(function() {"
                               "  if (document.body) {"
                               "    document.documentElement.style.overflow = 'hidden';"
                               "    document.body.innerHTML = %1[0];"
                               "    window.meow_cat = document.getElementById('meow_cat');"
                               "  }"
                               "})();"
                               ).arg(jsonArg);

        page()->runJavaScript(js);
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
            if (isBlankUrl(view()->url()))
            {
                drawMeowCat();
            }
            else
            {
                if (view()->url().host() != QUrl(SettingsPage::currentSearchEngineUrl()).host())
                {
                    if (view()->title().isEmpty())
                        updateCompleter(view()->url().toString(QUrl::RemoveScheme | QUrl::RemovePort), QString());
                    else
                        updateCompleter(view()->title(), view()->url().toString(QUrl::RemoveScheme | QUrl::RemovePort));
                }
                runUserScript();
            }
        }
    }
    else
    {
        qDebug() << "loading failed!!!";
    }
    applyIcon();
    ui->progressBar->setValue(0);
    ui->dockWidget->setWindowTitle("Web Inspector - " + (isBlankUrl(view()->url()) ? QStringLiteral("about:blank") : view()->url().toString()));
}

void WebPage::on_webView_titleChanged(const QString &)
{
    applyTitle();
}

void WebPage::on_webView_urlChanged(const QUrl &url)
{
    current_url = url;
    if (isBlankUrl(url))
    {
        ui->webAddress->clear();
    }
    else
    {
        ui->webAddress->setText(url.toString());
    }
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
    if (privacy == SettingsPage::IncognitoPrivacy || !parent() || !parent()->parent())
    {
        return;
    }
    if (QTabWidget *tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        const int index = tw->indexOf(this);
        QIcon icon = view()->icon();
        tw->setTabIcon(index, icon.isNull() ? windowIcon() : icon);
    }
}

void WebPage::applyTitle()
{
    QString title = current_page_title = view()->title();
    if (isBlankUrl(view()->url()))
    {
        title = "New Tab";
        ui->webAddress->clear();
    }
    if (!parent() || !parent()->parent())
        return;

    if (QTabWidget *tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        const int index = tw->indexOf(this);
        tw->setTabToolTip(index, title);
        tw->setTabText(index, title);
    }
}

void WebPage::runScript()
{
    bool ok = false;
    QString new_script = QInputDialog::getMultiLineText(topLevelWidget(), "Run Javascript code", "Javascript code", script, &ok);
    if (ok)
    {
        script = new_script;
        page()->runJavaScript(script);
    }
}

QString WebPage::queryElementAltAt(const QPoint &pos) const
{
    QString js = QString(
                     "(function() {"
                     "  var el = document.elementFromPoint(%1, %2);"
                     "  return (el && el.alt) ? String(el.alt) : '';"
                     "})()"
                     ).arg(pos.x()).arg(pos.y());
    return evalJsSync(js).toString();
}

void WebPage::customContextMenuRequested(const QPoint &pos)
{
    current_pos = pos;
    QMenu *menu = view()->createStandardContextMenu();
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();

    if (ctx && ctx->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage && !ctx->mediaUrl().isEmpty())
    {
        QAction *openImgAction = new QAction("Open Image in New Tab", menu);
        connect(openImgAction, &QAction::triggered, this, &WebPage::actionOpenImageInNewWindow);
        menu->addAction(openImgAction);
        menu->insertAction(openImgAction, ui->actionSearchImage);
    }
    if (!page()->selectedText().isEmpty())
    {
        menu->insertAction(menu->actions().value(0), ui->actionTranslateSelected);
    }
    menu->exec(view()->mapToGlobal(pos));
    menu->deleteLater();
}

void WebPage::actionOpenLink()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    QUrl url = ctx ? ctx->linkUrl() : QUrl();
    if (!url.isEmpty() && url.isValid())
    {
        if (mw && mw->lastPage())
            mw->lastPage()->load(url);
        else
            load(url);
    }
}

void WebPage::actionOpenLinkInNewWindow()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    QUrl url = ctx ? ctx->linkUrl() : QUrl();
    if (!url.isEmpty() && url.isValid())
    {
        emit newWindowLink(url);
    }
}

void WebPage::actionOpenImageInNewWindow()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    QUrl url = ctx ? ctx->mediaUrl() : QUrl();
    if (!url.isEmpty() && url.isValid())
    {
        emit newWindowLink(url);
    }
}

void WebPage::actionDownloadLinkToDisk()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    if (ctx)
    {
        current_alt_text = ctx->linkText();
        if (!ctx->linkUrl().isEmpty())
        {
            downloadRequest(CustomNetworkRequest(ctx->linkUrl()));
        }
    }
}

void WebPage::actionDownloadImageToDisk()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    if (ctx)
    {
        current_alt_text = queryElementAltAt(current_pos);
        if (!ctx->mediaUrl().isEmpty())
        {
            downloadRequest(CustomNetworkRequest(ctx->mediaUrl()));
        }
    }
}

void WebPage::handleProfileDownload(QWebEngineDownloadRequest *download)
{
    // Перехватываем только загрузки, относящиеся к нашей странице
    if (download->page() != page())
        return;

    QUrl url = download->url();
    download->cancel();
    downloadRequest(CustomNetworkRequest(url));
}

WebPage *WebPage::createHiddenWindow(QUrl url, Qt::ConnectionType conn) const
{
    qDebug() << "creating hidden page";
    WebPage *redirect_page = new WebPage(privacy, const_cast<WebPage *>(this));
    redirect_page->hide();
    connect(redirect_page, &WebPage::downloadFinished, this, &WebPage::networkDownloadFinished);
    connect(redirect_page, &WebPage::downloadProgress, this, &WebPage::updownProgress);
    redirect_page->fmanager->setStateFrom(fmanager);
    redirect_page->requested_url = url;
    redirect_page->temporary = true;
    if (!url.isEmpty())
    {
        QMetaObject::invokeMethod(redirect_page, "load", conn, Q_ARG(QUrl, url));
    }
    return redirect_page;
}

void WebPage::networkDownloadFinished(QNetworkReply *reply)
{
    qDebug() << "downloadFinished" << reply;
    fmanager->saveReplyToDisk(reply);
    ui->progressBar->setValue(0);
    emit downloadFinished(reply);
    if (temporary)
    {
        qDebug() << "deleting hidden page";
        deleteLater();
    }
}

void WebPage::updateCompleter(QString phrase, QString desc)
{
    if (privacy == SettingsPage::UsualPrivacy && !phrase.isEmpty())
    {
        if (phrase.startsWith("//"))
            phrase = phrase.mid(2);
        if (desc.startsWith("//"))
            desc = desc.mid(2);
        SettingsPage::addCompleterPhrase(phrase, desc);
    }
}

void WebPage::runUserScript()
{
    const QString us = SettingsPage::userScript();
    if (us.isEmpty())
        return;

    // Инжектируем во все фреймы документа
    QWebEngineScript scriptObj;
    scriptObj.setName("MeowUserScripts");
    scriptObj.setSourceCode(us);
    scriptObj.setInjectionPoint(QWebEngineScript::DocumentReady);
    scriptObj.setRunsOnSubFrames(true);
    scriptObj.setWorldId(QWebEngineScript::MainWorld);

    page()->scripts().remove(scriptObj);
    page()->scripts().insert(scriptObj);
    page()->runJavaScript(us);
}

void WebPage::downloadRequest(const QNetworkRequest &request)
{
    dmanager->downloadRequest(request);
}

void WebPage::featurePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
    qDebug() << securityOrigin << "requested" << feature;
    page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionDeniedByUser);
}

void WebPage::setIncognito()
{
    setWindowIcon(style()->standardIcon(QStyle::SP_VistaShield));
    QWebEngineSettings *settings = page()->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
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

WebPage *WebPage::createToolWindow(QUrl url, QString title, Qt::ConnectionType conn) const
{
    WebPage *new_webpage = new WebPage(privacy, const_cast<WebPage *>(this));
    new_webpage->setWindowTitle(title);
    new_webpage->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    for (int i = 0; i < new_webpage->ui->horizontalLayout->count(); i++)
    {
        QLayoutItem *item = new_webpage->ui->horizontalLayout->itemAt(i);
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
        ui->progressBar->setValue(static_cast<int>(ui->progressBar->maximum() * processed / total));
    }
}

void WebPage::inspectCurrentElement()
{
    if (inspector_view && inspector_view->page())
    {
        page()->setDevToolsPage(inspector_view->page());
    }
    ui->dockWidget->show();
}

void WebPage::on_actionSearchImage_triggered()
{
    const QWebEngineContextMenuRequest *ctx = view()->lastContextMenuRequest();
    QUrl url = ctx ? ctx->mediaUrl() : QUrl();
    if (!url.isEmpty() && url.isValid())
    {
        QString image_search_url = SettingsPage::currentImageSearchEngineUrl().arg(url.toString());
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
        text = ui->webAddress->selectedText();
    else if (!page()->selectedText().isEmpty())
        text = page()->selectedText();

    if (!text.isEmpty())
    {
        QString translate_url = SettingsPage::currentTranslateEngineUrl().arg(text);
        emit newWindowLink(QUrl(translate_url));
    }
}

void WebPage::on_webAddress_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = ui->webAddress->createStandardContextMenu();
    if (!ui->webAddress->selectedText().isEmpty() && !menu->actions().isEmpty())
    {
        menu->insertSeparator(menu->actions().front());
        menu->insertAction(menu->actions().front(), ui->actionTranslateSelected);
    }
    menu->exec(ui->webAddress->mapToGlobal(pos));
    menu->deleteLater();
}

bool WebPage::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}
