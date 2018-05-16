#include "filemanager.h"
#include "web_mime_types.h"
#include "custom_classes.h"
#include "utils.h"

#include <QStringList>
#include <QFileInfo>
#include <QFileDialog>
#include <QEventLoop>
#include <QProgressDialog>
#include <QBuffer>
#include <QImageReader>
#include <QtWebKit/QWebElement>

struct FileManager::SavePageStruct
{
    SavePageStruct(QString _path = QString())
    {
        activate(_path);
        switchRelative();
    }
    void activate(QString _path)
    {
        p_path = _path;
    }
    QString path() const
    {
        return p_path;
    }
    bool isActive() const
    {
        return !p_path.isEmpty();
    }
    void switchRelative()
    {
        p_root = false;
    }
    void switchRoot()
    {
        p_root = true;
    }
    bool isRoot() const
    {
        return p_root;
    }
    void reset()
    {
        p_path.clear();
    }
private:
    QString p_path;
    bool p_root;
};

FileManager::FileManager(QObject *parent) : QObject(parent), save_page(new SavePageStruct()), webpage(0)
{

}

FileManager::~FileManager()
{
    delete save_page;
}

void FileManager::setWebPage(WebPage * wp)
{
    webpage = wp;
}

bool FileManager::checkRedirect(QNetworkReply * reply) const
{
    const QString content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    const QString request_content_type = reply->request().header(QNetworkRequest::ContentTypeHeader).toString();
    return ((!content_type.startsWith(request_content_type) || request_content_type.isEmpty()) &&
            content_type.contains("text/html", Qt::CaseInsensitive) &&
            (!webpage || (webpage && !webpage->isHiddenWindow())) &&
            !save_page->isActive());
}

void FileManager::setStateFrom(FileManager * fm)
{
    * save_page = * fm->save_page;
}

QString FileManager::adjustFilename(QString filename, QString content, const QByteArray & data)
{
    QString known_content;
    if (filename.isEmpty())
    {
        filename = "index";
    }
    if (!content.isEmpty())
    {
        known_content = Meow::contentTypeForSuffix(QFileInfo(filename).suffix());
        if (QFileInfo(filename).suffix().isEmpty() || known_content.isEmpty() || (!known_content.isEmpty() && !content.startsWith(known_content, Qt::CaseInsensitive)))
        {
            QString known_suffix = Meow::suffixForContentType(content);
            if (!known_suffix.isEmpty())
            {
                filename += known_suffix;
//                qDebug() << "adjusted filename" << filename << "content" << Meow::adjustedContentType(content);
                return filename;
            }
        }
        else
        {
//            qDebug() << "adjusted filename" << filename << "content" << Meow::adjustedContentType(content);
            return filename;
        }
    }
    if (!QFileInfo(filename).suffix().isEmpty())
    {
        known_content = Meow::contentTypeForSuffix(QFileInfo(filename).suffix());
    }
    if (QFileInfo(filename).suffix().isEmpty() || known_content.isEmpty())
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
//    qDebug() << "adjusted filename" << filename;
    return filename;
}

void FileManager::modifyExif(QString filename, QUrl file_url, QUrl page_url, QStringList tags)
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

void FileManager::saveReplyToDisk(QNetworkReply * reply)
{
    if (!reply->isOpen())
    {
        qDebug() << "reply aborted";
        return;
    }
    if (!checkRedirect(reply))
    {
        saveReplyToDisk(reply->url(), reply->readAll(), reply->header(QNetworkRequest::ContentTypeHeader).toString());
    }
    else
    {
        qDebug() << "redirect detected" <<
                    reply->url() <<
                    reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (webpage)
        {
            webpage->createHiddenWindow(reply->url(), Qt::DirectConnection);
        }
    }
}

void FileManager::saveReplyToDisk(const QUrl & url, const QByteArray & data, QString content_type)
{
    qDebug() << "saveReplyToDisk" << url;

    QString url_adjusted = url.toString(QUrl::RemoveScheme | QUrl::RemovePort);
    QString filename = Meow::excludeUndesiredSymbols(QFileInfo(url_adjusted).fileName());
    filename = adjustFilename(filename, content_type, data);
    QString path;
    if (!save_page->isActive())
    {
        switch (SettingsPage::savePolicy())
        {
        case SettingsPage::SaveAsk:
            {
                QString save_file_name = QFileDialog::getSaveFileName(webpage ? webpage->topLevelWidget() : 0, "Download file \"" + filename + "\"", SettingsPage::currentSavePath() + "/" + filename);
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
                QString dir = QFileInfo(webpage ? webpage->currentUrl().toString(QUrl::RemoveScheme | QUrl::RemovePort) : "no_page").dir().path();
//                qDebug() << "download" << dir << filename;
                path = SettingsPage::savePath() + "/" + dir + "/";
                QDir().mkpath(path);
            }
            break;
        }
    }
    else
    {
        QString dir = QFileInfo(url_adjusted).dir().path();
//        QString dir = QFileInfo(current_url.toString(QUrl::RemoveScheme | QUrl::RemovePort)).dir().path();
        path = save_page->path() + "/";
        if (!save_page->isRoot())
        {
            path += dir +"/";
        }
        QDir().mkpath(path);
        int i = 2;
        QString src_filename = filename;
        while (QFile::exists(path + filename))
        {
            filename = QFileInfo(src_filename).baseName() + "_" + QString::number(i) + "." + QFileInfo(src_filename).completeSuffix();
            i++;
        }
    }

    last_file_name = QString(path + filename).replace(QRegExp("[\\/]+"), "/");
    QFile file(last_file_name);
    if (!data.isEmpty() && file.open(QFile::WriteOnly))
    {
        file.write(data);
        file.close();

        SettingsPage::addDownloadEntry(
                    file.fileName(),
                    url.toString(),
                    webpage ? webpage->currentUrl().toString() : "",
                    file.size(),
                    webpage ? webpage->currentPageTitle() : "",
                    webpage ? webpage->currentAltText() : ""
                    );

        if (SettingsPage::writeMetadataEnabed() && content_type.startsWith("image", Qt::CaseInsensitive))
        {
            modifyExif(
                        file.fileName(),
                        url,
                        webpage ? webpage->currentUrl() : QUrl(),
                        QStringList() <<
                            Meow::excludeUndesiredSymbols(webpage ? webpage->currentPageTitle() : "") <<
                            Meow::excludeUndesiredSymbols(webpage ? webpage->currentAltText() : "") <<
                            Meow::excludeUndesiredSymbols(webpage ? webpage->currentUrl().host() : "") <<
                            Meow::excludeUndesiredSymbols(url.host())
                        );
        }

        if (SettingsPage::savePolicy() == SettingsPage::SaveSilent && !save_page->isActive())
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

QString FileManager::parseCssBgImageUrl(const QString & css, int & start, int & end)
{
    QRegExp re("url\\s*\\(\\s*[\\/:\"'&?=\\.\\+\\-%\\w]*\\s*\\)");
    start = re.indexIn(css, start);
    if (start < 0)
    {
        end = -1;
        return QString();
    }
    end = start + re.matchedLength();
    return css.mid(start, end - start).replace(QRegExp("url\\s*\\(\\s*[\"']*"), "").replace(QRegExp("\\s*[\"']*\\s*\\)"), "");
}

QString FileManager::savePageElement(QString tag, QString attr, QString value)
{
    if (!webpage || value.isEmpty() ||
            (tag.compare("img", Qt::CaseInsensitive) == 0 && value.endsWith(".ico", Qt::CaseInsensitive)) ||
            (tag.compare("a", Qt::CaseInsensitive) == 0 && value == "#"))
    {
        return QString();
    }
    if (attr == "style")
    {
        int start = 0, end = 0;
        value = parseCssBgImageUrl(value, start, end);
        if (start == -1 || value.isEmpty())
        {
            return QString();
        }
    }
    QUrl url(value);
    qDebug() << tag << attr << value << url << url.host();
    if (url.host().isEmpty())
    {
        url = QUrl("//" + webpage->currentUrl().host() + url.toString());
    }
    if (url.scheme().isEmpty())
    {
        url.setScheme(webpage->currentUrl().scheme());
    }
    if (tag.compare("a", Qt::CaseInsensitive) == 0 /*|| tag.compare("link", Qt::CaseInsensitive) == 0*/)
    {
        return url.toString();
    }
    qDebug() << tag << attr << value << url;
    QEventLoop loop;
    connect(webpage, SIGNAL(downloadFinished()), & loop, SLOT(quit()));
    webpage->downloadRequest(CustomNetworkRequest(url));
    loop.exec();
    QDir().mkpath(QFileInfo(last_file_name).absolutePath());
    return QDir(save_page->path()).relativeFilePath(last_file_name);
}

void FileManager::savePage()
{
    if (!webpage)
    {
        return;
    }
    qDebug() << "save page";
    save_page->switchRelative();
    if (SettingsPage::savePolicy() == SettingsPage::SaveAsk)
    {
        QString path = QFileDialog::getExistingDirectory(webpage->topLevelWidget(), "Folder to save page", SettingsPage::currentSavePath());
        if (path.isEmpty())
        {
            return;
        }
        SettingsPage::setCurrentSavePath(path);
        save_page->activate(path);
    }
    else
    {
        save_page->activate(SettingsPage::savePath() + "/pages/" + Meow::excludeUndesiredSymbols(webpage->currentPageTitle()) + "/");
    }
    webpage->setEnabled(false);
    QProgressDialog * progress = new QProgressDialog(webpage->view());
    progress->setWindowTitle("Saving page");
    progress->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowCloseButtonHint);
    progress->setMinimumDuration(500);
    webpage->view()->stop();
    QDir().mkpath(save_page->path());
    QWebElement document = webpage->page()->mainFrame()->documentElement();
    QMultiMap< QString, QList<QWebElement> > map;
    QList<QWebElement> list;
    int count = 0;
    list = document.findAll("img").toList();
    map.insertMulti("src", list); count += list.count();
    list = document.findAll("script").toList();
    map.insertMulti("src", list); count += list.count();
    list = document.findAll("link").toList();
    map.insertMulti("href", list); count += list.count();
    list = document.findAll("a").toList();
    map.insertMulti("href", list); count += list.count();
    map.insertMulti("style", list); count += list.count();
    list = document.findAll("div").toList();
    map.insertMulti("style", list); count += list.count();
    list = document.findAll("span").toList();
    map.insertMulti("style", list); count += list.count();
    progress->setMaximum(0);
    progress->setWindowModality(Qt::NonModal);

    foreach (QWebElement el, document.findAll("style").toList())
    {
        if (progress->wasCanceled())
        {
            break;
        }
        QString css = el.toPlainText();
        int end = 0;
        while (end >= 0)
        {
            if (progress->wasCanceled())
            {
                break;
            }
            int start = end;
            QString url = parseCssBgImageUrl(css, start, end);
            if (start >= 0 && !url.isEmpty())
            {
                QString res = savePageElement(QString(), QString(), url);
                if (!res.isEmpty())
                {
                    css = css.remove(start, end - start).insert(start, "url('" + res + "')");
                    end += (5 + res.length() + 2) - (end - start);
                }
            }
        }

        el.setPlainText(css);
    }

    progress->setMaximum(count);

    int k = 0;
    for (QMultiMap< QString, QList<QWebElement> >::iterator i = map.begin(); i != map.end(); ++i)
    {
        if (progress->wasCanceled())
        {
            break;
        }
        QString url_attr = i.key();
        foreach (QWebElement el, i.value())
        {
            progress->setValue(k++);
            QString res = savePageElement(el.tagName(), url_attr, el.attribute(url_attr));
            if (!res.isEmpty())
            {
                el.setAttribute(url_attr, res);
            }
        }
    }
    save_page->switchRoot();
    saveReplyToDisk(Meow::excludeUndesiredSymbols(webpage->currentPageTitle()) + ".html", webpage->page()->mainFrame()->toHtml().toUtf8(), "text/html");
//    QString full_path = "file:///" + save_page.path + Meow::excludeUndesiredSymbols(current_page_title) + ".html";
    QString full_path = "file:///" + last_file_name;
    save_page->reset();
    webpage->load(full_path);
    progress->hide();
    progress->deleteLater();
    webpage->setEnabled(true);
}
