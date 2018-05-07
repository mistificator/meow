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
#include <QtWebKit/QWebElement>
#include <QFileDialog>
#include <QImageReader>
#include <QBuffer>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QNetworkConfiguration>

#include "mainwindow.h"
#include "tabbar.h"

class UserAgentWebPage : public QWebPage
{
public:
    UserAgentWebPage(QObject * _parent = 0): QWebPage(_parent) {}
protected:
    virtual QString userAgentForUrl(const QUrl &) const
    {
        QString str = SettingsPage::useragentString();
        if (str.isEmpty())
        {
            SettingsPage::setUseragentString(str = QWebPage::userAgentForUrl(QUrl("https://example.com")));
        }
        return str;
    }
};

class CompleterColumnDelegate: public QStyledItemDelegate
{
public:
    CompleterColumnDelegate(QObject * _parent = 0): QStyledItemDelegate(_parent) {}
protected:
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        if (index.column() == SettingsPage::completerHistoryModel()->fieldIndex("Description"))
        {
            opt.palette.setColor(QPalette::Text, QColor(Qt::gray));
            opt.font.setItalic(true);
        }

        QStyledItemDelegate::paint(painter, opt, index);
    }
};

class UrlCompleter: public QCompleter
{
public:
    UrlCompleter(QObject * _parent): QCompleter(_parent) {}
    QString pathFromIndex(const QModelIndex & index) const
    {
        QSqlTableModel * model = SettingsPage::completerHistoryModel();
        QString desc = model->data(model->index(index.row(), model->fieldIndex("Description"))).toString();
        if (!desc.isEmpty())
        {
            return desc;
        }
        return index.data().toString();
    }
};

WebPage::WebPage(Privacy _privacy, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebPage),
    privacy(_privacy),
    script("// sample script\r\ndocument.title = \"This is the new page title.\";")
{
    ui->setupUi(this);

    view()->setPage(new UserAgentWebPage());    
    page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    page()->setForwardUnsupportedContent(true);

    connect(page(), SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(page(), SIGNAL(linkHovered(QString,QString,QString)), this, SIGNAL(linkHovered(QString,QString,QString)));
    connect(page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(initialLayoutCompleted()));
    connect(page(), SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadRequested(QNetworkRequest)));
    connect(page(), SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(unsupportedContent(QNetworkReply*)));
    connect(page(), SIGNAL(loadProgress(int)), ui->progressBar, SLOT(setValue(int)));
    connect(page(), SIGNAL(frameCreated(QWebFrame*)), this, SLOT(frameCreated(QWebFrame*)));

    inspector = new QWebInspector();
    inspector->setPage(page());
    ui->dockWidget->setWidget(inspector);
    ui->dockWidget->hide();

    connect(this, SIGNAL(showNotifier(QString, int)), dynamic_cast<MainWindow *>(topLevelWidget()), SLOT(showNotifier(QString, int)));

    manager = new QNetworkAccessManager(this);    
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));

    connect(view(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
    connect(view()->pageAction(QWebPage::OpenLink), SIGNAL(triggered()), this, SLOT(actionOpenLink()));
    connect(view()->pageAction(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenLinkInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenFrameInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenFrameInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenImageInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenImageInNewWindow()));
    connect(view()->pageAction(QWebPage::DownloadLinkToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadLinkToDisk()));
    connect(view()->pageAction(QWebPage::DownloadImageToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadImageToDisk()));
    connect(view()->pageAction(QWebPage::InspectElement), SIGNAL(triggered()), this, SLOT(actionInspectElement()));

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

    if (privacy == IncognitoPrivacy)
    {
        setIncognito();
    }
    else
    {
        setWindowIcon(style()->standardIcon(QStyle::SP_DesktopIcon));
    }
}

WebPage::~WebPage()
{
    delete ui;
}

QWebView * WebPage::view() const
{
    return ui->webView;
}

QWebPage * WebPage::page() const
{
    return view()->page();
}

void WebPage::on_actionLoadPage_triggered()
{
    QUrl url(ui->webAddress->text());
    QUrl url_copy(url);
    if (url.toString().trimmed().compare("about:blank", Qt::CaseInsensitive) != 0)
    {
        if (!url.toString().contains("."))
        {
            QString search_engine = SettingsPage::currentSearchEngineUrl();
            if (search_engine.contains("%1"))
            {
                url = search_engine.arg(QString(url_copy.toEncoded()));
                updateCompleter(url_copy.toString(QUrl::RemoveScheme | QUrl::RemovePort), "");
            }
            else
            if (!search_engine.isEmpty())
            {
                url = search_engine + url_copy.toEncoded();
                updateCompleter(url_copy.toString(QUrl::RemoveScheme | QUrl::RemovePort), "");
            }
            else
            {
                url = "0.0.0.0";
            }
        }
        if ((privacy == IncognitoPrivacy && url.scheme().compare("http", Qt::CaseInsensitive) == 0) || url.scheme().isEmpty())
        {
            url.setScheme("https");
        }
    }
    view()->load(current_url = url);
}

void WebPage::on_actionReloadPage_triggered()
{
    if (view()->url() == current_url)
    {
        view()->triggerPageAction(QWebPage::ReloadAndBypassCache);
    }
    else
    {
        view()->load(current_url);
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

void WebPage::on_webView_loadFinished(bool status)
{
    if (status)
    {
        if (property("temporary").toBool() && property("requested_url").toUrl() != view()->url())
        {
            downloadRequested(QNetworkRequest(view()->url()));
        }
        else
        {
            runUserScriptRecursive(page()->mainFrame());
        }
    }
    else
    {
        qDebug() << "loading failed!!!";
//        page()->mainFrame()->setHtml("<html><body><div style=\"position: absolute; top: 0;bottom: 0; left: 0; right: 0; margin: auto; width: 200px; height: 100px;\"><h1>Loading failed</h1></div></body></html>", view()->url());
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
    if (isIncognito())
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
        if (TabBar * tb = qobject_cast<TabBar *>(TabWidgetOpener::tabBar(tw)))
        {
            const int index = tw->indexOf(this);
            tw->setTabToolTip(index, title);
            tw->setTabText(index, title);
        }
    }
}

#include <QInputDialog>
void WebPage::customContextMenuRequested(const QPoint & pos)
{
    QMenu * menu = page()->createStandardContextMenu();
    menu->addSeparator();
    QAction * js_action = isIncognito() ? 0 : menu->addAction("Run script");
    QAction * result = menu->exec(mapToGlobal(current_pos = pos));
    if (js_action && result == js_action)
    {
        bool ok = false;
        QString new_script;
        page()->mainFrame()->evaluateJavaScript(new_script = QInputDialog::getMultiLineText(topLevelWidget(), "Run Javascript code", "Javascript code", script, & ok));
        if (ok)
        {
            script = new_script;
        }
    }
    menu->deleteLater();
}

QWebHitTestResult WebPage::hitTest(QPoint pos) const
{
    return view()->page()->mainFrame()->hitTestContent(pos);
}

void WebPage::linkClicked(QUrl url)
{
    current_pos = view()->mapFromGlobal(QCursor::pos());
    view()->pageAction(QWebPage::OpenLink)->trigger();
}

void WebPage::actionOpenLink()
{
    QWebHitTestResult hit_test = hitTest(current_pos);
    QUrl url = hit_test.linkUrl();
    if (!url.isEmpty() && url.isValid())
    {
        QWebElement element = hit_test.element();
        while (!element.isNull() && element.tagName().compare("a", Qt::CaseInsensitive) != 0)
        {
            element = element.parent();
        }
        if (element.attribute("target").contains("blank", Qt::CaseInsensitive))
        {
            emit newWindowLink(url);
        }
        else
        {
            view()->load(url);
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
    QString html = hitTest(current_pos).frame()->documentElement().toOuterXml();
    if (!html.isEmpty())
    {
        WebPage * frame_page = new WebPage(privacy, topLevelWidget());
        frame_page->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
        frame_page->setWindowTitle(current_page_title);
        frame_page->view()->setHtml(html);
        for (int i = 0; i < frame_page->ui->horizontalLayout->count(); i++)
        {
            QLayoutItem * item = frame_page->ui->horizontalLayout->itemAt(i);
            if (item->widget())
            {
                item->widget()->hide();
            }
        }
        ui->progressBar->hide();
        frame_page->show();
        frame_page->activateWindow();
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

QString WebPage::adjustFilename(QString filename, const QByteArray & data)
{
    if (filename.isEmpty())
    {
        filename = "index";
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        QBuffer buf;
        buf.setData(data);
        QImageReader img(& buf);
        if (img.canRead())
        {
            filename += "." + img.format();
        }
        else
        if (data.trimmed().startsWith("<"))
        {
            filename += ".html";
        }
    }
    return filename;
}

void WebPage::modifyExif(QString filename, QUrl file_url, QUrl page_url, QStringList tags)
{
    tags.removeDuplicates();
    if (fmeta.setImage(filename))
    {        
        qDebug() << filename << tags << file_url << page_url;
        if (
                fmeta.setField(FileMetadata::TagXPKeywords, tags.join("; ")) &&
                fmeta.setField(FileMetadata::TagExifUserComment, file_url.toString() + "\n\n" + page_url.toString())
            )
        {
            qDebug() << "properties of" << QFileInfo(filename).fileName() << "updated";
        }
        else
        {
            qDebug() << "properties of" << QFileInfo(filename).fileName() << "NOT updated";
        }
    }
    else
    {
        qDebug() << "couldn't open" << QFileInfo(filename).fileName() << "to update properties";
    }
}

void WebPage::saveReplyToDisk(QNetworkReply * reply)
{
    const QByteArray & data = reply->readAll();
    QUrl url = reply->url();

    if (data.trimmed().startsWith("<"))
    {
        qDebug() << "redirect detected" << url;
        qDebug() << "creating hidden page";
        WebPage * redirect_page = new WebPage(privacy, this);
        redirect_page->hide();
        redirect_page->setProperty("requested_url", url);
        redirect_page->setProperty("temporary", true);
        redirect_page->view()->load(url);
        return;
    }
    qDebug() << "saveReplyToDisk" << url;

    QString url_adjusted = url.toString(QUrl::RemoveScheme | QUrl::RemovePort);
    QString filename = QFileInfo(url_adjusted).fileName();
    if (filename.contains(undesiredSymbols()))
    {
        filename.clear();
    }

    filename = adjustFilename(filename, data);
    QString path;
    switch (SettingsPage::savePolicy())
    {
    case SettingsPage::SaveAsk:
        {
            QString save_file_name = QFileDialog::getSaveFileName(topLevelWidget(), "Download file \"" + filename + "\"", SettingsPage::currentSavePath() + "/" + filename);
            if (save_file_name.isEmpty())
            {
                return;
            }
            path = QFileInfo(save_file_name).dir().path() + "/";
            filename = QFileInfo(save_file_name).fileName();
            SettingsPage::setCurrentSavePath(path);
        }
        break;
    case SettingsPage::SaveSilent:
        {
        //    QString dir = QFileInfo(url_adjusted).dir().path();
            QString dir = QFileInfo(current_url.toString(QUrl::RemoveScheme | QUrl::RemovePort)).dir().path();
            qDebug() << "download" << dir << filename << reply->isFinished();
            path = SettingsPage::savePath() + "/" + dir + "/";
            QDir().mkpath(path);
        }
        break;
    }

    QFile file(path + filename);
    if (!data.isEmpty() && file.open(QFile::WriteOnly))
    {
        file.write(data);
        file.close();

        SettingsPage::addDownloadEntry(file.fileName(), url.toString(), current_url.toString(), file.size(), current_page_title, current_alt_text);

        if (SettingsPage::writeMetadataEnabed())
        {
            modifyExif(
                        file.fileName(),
                        url,
                        current_url,
                        QStringList() <<
                            current_page_title.replace(undesiredSymbols(), QString()) <<
                            current_alt_text.replace(undesiredSymbols(), QString()) <<
                            current_url.host().replace(undesiredSymbols(), QString()) <<
                            url.host().replace(undesiredSymbols(), QString())
                        );
        }

        if (SettingsPage::savePolicy() == SettingsPage::SaveSilent)
        {
            QString link;
            int count = 0;
            do
            {
                link = SettingsPage::savePath() + "/" + filename + (count > 0 ? "(" + QString::number(count) + ")" : "");
        #ifdef Q_OS_WIN
                link += ".lnk";
        #endif
                count++;
            }
            while (QFile::exists(link));
            file.link(link);
        }

        emit showNotifier("\"" + filename + "\" downloaded");
    }
    else
    {
        emit showNotifier("\"" + filename + "\" download failed");
    }
}

void WebPage::replyFinished(QNetworkReply * reply)
{
    qDebug() << "replyFinished";
    saveReplyToDisk(reply);
    delete reply;

    if (property("temporary").toBool())
    {
        qDebug() << "deleting hidden page";
        deleteLater();
    }
    ui->progressBar->setValue(0);
}

void WebPage::updateCompleter(QString phrase, QString desc)
{
    if (!isIncognito())
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

void WebPage::frameCreated(QWebFrame *)
{
}

void WebPage::downloadRequested(const QNetworkRequest & request)
{
    qDebug() << "request" << request.url();
    QNetworkReply * reply = manager->get(request);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updownProgress(qint64, qint64)));
    qDebug() << "downloading started";
}

void WebPage::unsupportedContent(QNetworkReply * reply)
{
    qDebug() << "unsupported content";
    downloadRequested(QNetworkRequest(reply->url()));
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

bool WebPage::isIncognito() const
{
    return privacy == IncognitoPrivacy;
}

QRegExp WebPage::undesiredSymbols(QString str) const
{
    return QRegExp("[" + str + "]");
}

void WebPage::updownProgress(qint64 processed, qint64 total)
{
    qDebug() << "updown" << processed << "/" << total;
    if (total > 0)
    {
        ui->progressBar->setValue(ui->progressBar->maximum() * processed / total);
    }
}

void WebPage::actionInspectElement()
{
    ui->dockWidget->show();
}

