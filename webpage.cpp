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

#include "mainwindow.h"
#include "settingspage.h"

class UserAgentWebPage : public QWebPage
{
public:
    UserAgentWebPage(QObject * _parent = 0): QWebPage(_parent) {}
protected:
    virtual QString userAgentForUrl(const QUrl & url ) const
    {
//        qDebug() << QWebPage::userAgentForUrl(url);
        return QWebPage::userAgentForUrl(url);
    }
};

WebPage::WebPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WebPage)
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

    connect(this, SIGNAL(showNotifier(QString, int)), dynamic_cast<MainWindow *>(topLevelWidget()), SLOT(showNotifier(QString, int)));

    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    connect(view(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
    connect(view()->pageAction(QWebPage::OpenLink), SIGNAL(triggered()), this, SLOT(actionOpenLink()));
    connect(view()->pageAction(QWebPage::OpenLinkInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenLinkInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenFrameInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenFrameInNewWindow()));
    connect(view()->pageAction(QWebPage::OpenImageInNewWindow), SIGNAL(triggered()), this, SLOT(actionOpenImageInNewWindow()));
    //connect(view()->pageAction(QWebPage::DownloadLinkToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadLinkToDisk()));
    //connect(view()->pageAction(QWebPage::DownloadImageToDisk), SIGNAL(triggered()), this, SLOT(actionDownloadImageToDisk()));

    ui->actionLoadPage->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    ui->actionReloadPage->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));

    ui->loadPageButton->setDefaultAction(ui->actionLoadPage);
    ui->reloadButton->setDefaultAction(ui->actionReloadPage);
    ui->backButton->setDefaultAction(ui->actionBack);
    ui->forwardButton->setDefaultAction(ui->actionForward);
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
            }
            else
            if (!search_engine.isEmpty())
            {
                url = search_engine + url_copy.toEncoded();
            }
            else
            {
                url = "0.0.0.0";
            }
        }
        if (url.scheme().isEmpty())
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
//        view()->reload();
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
    }
    else
    {
        qDebug() << "loading failed!!!";
        page()->mainFrame()->setHtml("<html><body><div style=\"position: absolute; top: 0;bottom: 0; left: 0; right: 0; margin: auto; width: 200px; height: 100px;\"><h1>Loading failed</h1></div></body></html>");
    }
    applyIcon();
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
    if (QTabWidget * tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        const int index = tw->indexOf(this);
        QIcon icon = QWebSettings::iconForUrl(current_url);
        tw->setTabIcon(index, icon.isNull() ? style()->standardIcon(QStyle::SP_ComputerIcon) : icon);
    }
}

void WebPage::applyTitle()
{
    if (QTabWidget * tw = qobject_cast<QTabWidget *>(parent()->parent()))
    {
        const int index = tw->indexOf(this);
        QString title = view()->title();
        tw->setTabToolTip(index, title);
        while (!title.isEmpty() && tw->fontMetrics().width(title) > 90)
        {
            title = title.left(title.count() - 1);
        }
        if (title != view()->title())
        {
            title += "...";
        }
        if (view()->url().isEmpty() || view()->url().toString().trimmed().compare("about:blank", Qt::CaseInsensitive) == 0)
        {
            title = "New Tab";
            ui->webAddress->setText("");
        }
        while (tw->fontMetrics().width(title) < 100)
        {
            title = title + " ";
        }
        tw->setTabText(index, title);
    }
}

void WebPage::customContextMenuRequested(const QPoint & pos)
{
    QMenu * menu = page()->createStandardContextMenu();
    menu->exec(mapToGlobal(current_pos = pos));
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
    QUrl url = hitTest(current_pos).frame()->url();
    if (!url.isEmpty() && url.isValid())
    {
        emit newWindowLink(url);
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
/*
void WebPage::actionDownloadLinkToDisk()
{
    QUrl url = hitTest(current_pos).linkUrl();
    if (!url.isEmpty() && url.isValid())
    {
        downloadRequested(QNetworkRequest(url));
    }
}

void WebPage::actionDownloadImageToDisk()
{
    QUrl url = hitTest(current_pos).imageUrl();
    if (!url.isEmpty() && url.isValid())
    {
        downloadRequested(QNetworkRequest(url));
    }
}
*/
QString WebPage::adjustFilename(QString filename, const QByteArray & data)
{
    if (filename.isEmpty())
    {
        filename = "index";
    }
    if (QFileInfo(filename).suffix().isEmpty())
    {
        if (data.startsWith("\xff\xd8\xff\xe0") || data.startsWith("\xff\xd8\xff\xe1"))
        {
            filename += ".jpg";
        }
        else
        if (data.startsWith("\x89PNG"))
        {
            filename += ".png";
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
    if (fmeta.setImage(filename))
    {
        if (
                fmeta.setField(FileMetadata::TagXPKeywords, tags.join("; ").toLocal8Bit()) &&
                fmeta.setField(FileMetadata::TagXPComment, file_url.toEncoded() + "\n\n" + page_url.toEncoded())
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
    QByteArray data = reply->readAll();
    QUrl url = reply->url();

    if (data.trimmed().startsWith("<"))
    {
        qDebug() << "redirect detected" << url;
        qDebug() << "creating hidden page";
        WebPage * redirect_page = new WebPage(this);
        redirect_page->hide();
        redirect_page->setProperty("requested_url", url);
        redirect_page->setProperty("temporary", true);
        redirect_page->view()->load(url);
        return;
    }
    QString url_adjusted = url.toString(QUrl::RemoveScheme | QUrl::RemovePort);
    QString filename = QFileInfo(url_adjusted).fileName();
    if (filename.contains(QRegExp("[&,\\:?]")))
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

        if (SettingsPage::writeMetadataEnabed())
        {
            modifyExif(
                        file.fileName(),
                        url,
                        current_url,
                        QStringList() <<
                            url.toString(QUrl::RemoveScheme | QUrl::RemovePort | QUrl::RemovePath | QUrl::RemoveQuery) <<
                            current_url.toString(QUrl::RemoveScheme | QUrl::RemovePort | QUrl::RemovePath | QUrl::RemoveQuery));
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
    saveReplyToDisk(reply);
    delete reply;

    if (property("temporary").toBool())
    {
        qDebug() << "deleting hidden page";
        deleteLater();
    }
}

void WebPage::initialLayoutCompleted()
{
    update();
}

void WebPage::downloadRequested(const QNetworkRequest & request)
{
    qDebug() << "request" << request.url();
    manager->get(request);
}

void WebPage::unsupportedContent(QNetworkReply * reply)
{
    qDebug() << "unsupported content";
    downloadRequested(QNetworkRequest(reply->url()));
    delete reply;
}

void WebPage::featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature)
{
    page()->setFeaturePermission(frame, feature, QWebPage::PermissionDeniedByUser);
}

