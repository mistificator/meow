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
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

struct FileManager::SavePageStruct
{
    SavePageStruct(QString _path = QString())
    {
        activate(_path);
        switchRelative();
    }
    void activate(QString _path) { p_path = _path; }
    QString path() const { return p_path; }
    bool isActive() const { return !p_path.isEmpty(); }
    void switchRelative() { p_root = false; }
    void switchRoot() { p_root = true; }
    bool isRoot() const { return p_root; }
    void reset() { p_path.clear(); }

private:
    QString p_path;
    bool p_root;
};

FileManager::FileManager(QObject *parent)
    : QObject(parent), webpage(nullptr), save_page(new SavePageStruct())
{
}

FileManager::~FileManager()
{
    delete save_page;
}

void FileManager::setWebPage(WebPage *wp)
{
    webpage = wp;
}

bool FileManager::checkRedirect(QNetworkReply *reply) const
{
    const QString content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    const QString request_content_type = reply->request().header(QNetworkRequest::ContentTypeHeader).toString();
    return ((!content_type.startsWith(request_content_type) || request_content_type.isEmpty()) &&
            content_type.contains("text/html", Qt::CaseInsensitive) &&
            (!webpage || (webpage && !webpage->isHiddenWindow())) &&
            !save_page->isActive());
}

void FileManager::setStateFrom(FileManager *fm)
{
    *save_page = *fm->save_page;
}

QString FileManager::adjustFilename(QString filename, QString content, const QByteArray &data)
{
    QString known_content;
    if (filename.isEmpty())
    {
        filename = "index";
    }
    if (!content.isEmpty())
    {
        known_content = Meow::contentTypeForSuffix(QFileInfo(filename).suffix());
        if (QFileInfo(filename).suffix().isEmpty() || known_content.isEmpty() ||
            (!known_content.isEmpty() && !content.startsWith(known_content, Qt::CaseInsensitive)))
        {
            QString known_suffix = Meow::suffixForContentType(content);
            if (!known_suffix.isEmpty())
            {
                filename += known_suffix;
                return filename;
            }
        }
        else
        {
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
        QImageReader img(&buf);
        if (img.canRead())
        {
            filename += "." + QString::fromLatin1(img.format());
        }
        else if (data.trimmed().startsWith("<"))
        {
            filename += ".html";
        }
    }
    return filename;
}

void FileManager::modifyExif(QString filename, QUrl file_url, QUrl page_url, QStringList tags)
{
    tags.removeDuplicates();
    if (fmeta.setImage(filename))
    {
        if (fmeta.setField(FileMetadata::TagXPKeywords, tags.join("; ")) &&
            fmeta.setField(FileMetadata::TagExifUserComment, file_url.toString() + "\n\n" + page_url.toString()))
        {
            qDebug() << "properties of" << QFileInfo(filename).fileName() << "updated";
        }
    }
}

void FileManager::saveReplyToDisk(QNetworkReply *reply)
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
        if (webpage)
        {
            webpage->createHiddenWindow(reply->url(), Qt::DirectConnection);
        }
    }
}

void FileManager::saveReplyToDisk(const QUrl &url, const QByteArray &data, QString content_type)
{
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
            QString save_file_name = QFileDialog::getSaveFileName(
                webpage ? webpage->topLevelWidget() : nullptr,
                "Download file \"" + filename + "\"",
                SettingsPage::currentSavePath() + "/" + filename);
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
            QString dir = QFileInfo(webpage ? webpage->currentUrl().toString(QUrl::RemoveScheme | QUrl::RemovePort) : "no_page").dir().path();
            path = SettingsPage::savePath() + "/" + dir + "/";
            QDir().mkpath(path);
        }
        break;
        }
    }
    else
    {
        QString dir = QFileInfo(url_adjusted).dir().path();
        path = save_page->path() + "/";
        if (!save_page->isRoot())
        {
            path += dir + "/";
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

    last_file_name = QString(path + filename).replace(QRegularExpression("[\\\\/]+"), "/");
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
            webpage ? webpage->currentAltText() : "");

        if (SettingsPage::writeMetadataEnabed() && content_type.startsWith("image", Qt::CaseInsensitive))
        {
            modifyExif(
                file.fileName(),
                url,
                webpage ? webpage->currentUrl() : QUrl(),
                QStringList()
                    << Meow::excludeUndesiredSymbols(webpage ? webpage->currentPageTitle() : "")
                    << Meow::excludeUndesiredSymbols(webpage ? webpage->currentAltText() : "")
                    << Meow::excludeUndesiredSymbols(webpage ? webpage->currentUrl().host() : "")
                    << Meow::excludeUndesiredSymbols(url.host()));
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
            } while (QFile::exists(link));
            file.link(link);
        }
        emit showNotifier("\"" + filename + "\" downloaded");
    }
    else
    {
        emit showNotifier("\"" + filename + "\" download failed");
    }
}

QString FileManager::parseCssBgImageUrl(const QString &css, int &start, int &end)
{
    static const QRegularExpression re("url\\s*\\(\\s*[\\/:\"'&?=\\.\\+\\-%\\w]*\\s*\\)");
    QRegularExpressionMatch match = re.match(css, start);
    if (!match.hasMatch())
    {
        start = -1;
        end = -1;
        return QString();
    }
    start = match.capturedStart();
    end = match.capturedEnd();
    QString raw = match.captured(0);
    raw.replace(QRegularExpression("^url\\s*\\(\\s*[\"']*"), "");
    raw.replace(QRegularExpression("\\s*[\"']*\\s*\\)$"), "");
    return raw;
}

QString FileManager::savePageElement(QString tag, QString attr, QString value)
{
    if (!webpage || value.isEmpty() ||
        (tag.compare("a", Qt::CaseInsensitive) == 0 && !QUrl(value).isValid()))
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

    // Исправлено разрешение относительных URL
    QUrl url = webpage->currentUrl().resolved(QUrl(value));
    if (!url.isValid() || url.scheme().startsWith("data", Qt::CaseInsensitive) || url.scheme().startsWith("javascript", Qt::CaseInsensitive))
    {
        return QString();
    }
    if (tag.compare("a", Qt::CaseInsensitive) == 0)
    {
        return url.toString();
    }

    last_file_name.clear();
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(webpage, &WebPage::downloadFinished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    webpage->downloadRequest(CustomNetworkRequest(url));
    timer.start(30000);
    loop.exec();

    if (last_file_name.isEmpty())
        return QString();

    QDir().mkpath(QFileInfo(last_file_name).absolutePath());
    return QDir(save_page->path()).relativeFilePath(last_file_name);
}

void FileManager::savePage()
{
    if (!webpage)
        return;

    if (SettingsPage::savePolicy() == SettingsPage::SaveAsk)
    {
        QString path = QFileDialog::getExistingDirectory(webpage->topLevelWidget(), "Folder to save page", SettingsPage::currentSavePath());
        if (path.isEmpty())
            return;
        SettingsPage::setCurrentSavePath(path);
        save_page->activate(path);
    }
    else
    {
        save_page->activate(SettingsPage::savePath() + "/pages/" + Meow::excludeUndesiredSymbols(webpage->currentPageTitle()) + "/");
    }

    webpage->view()->stop();
    webpage->setEnabled(false);
    save_page->switchRelative();
    QDir().mkpath(save_page->path());

    QProgressDialog *progress = new QProgressDialog(webpage->view());
    progress->setWindowTitle("Saving page");
    progress->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowCloseButtonHint);
    progress->setMinimumDuration(500);
    progress->setWindowModality(Qt::NonModal);

    // 1. Собираем через JS все ресурсы из DOM (img, script, link, a, div, span, iframe, style)
    const QString collectJs = R"JS(
        (function() {
            var items = [];
            var selectors = [
                ['img', 'src'], ['script', 'src'], ['link', 'href'],
                ['a', 'href'], ['a', 'style'], ['div', 'style'],
                ['span', 'style'], ['iframe', 'src']
            ];
            selectors.forEach(function(pair) {
                var els = document.querySelectorAll(pair[0]);
                for (var i = 0; i < els.length; i++) {
                    var val = els[i].getAttribute(pair[1]);
                    if (val) items.push({tag: pair[0], attr: pair[1], val: val});
                }
            });
            var styles = [];
            var styleEls = document.querySelectorAll('style');
            for (var j = 0; j < styleEls.length; j++) {
                styles.push(styleEls[j].textContent || '');
            }
            return JSON.stringify({items: items, styles: styles});
        })()
    )JS";

    QJsonDocument doc = QJsonDocument::fromJson(webpage->evalJsSync(collectJs).toString().toUtf8());
    QJsonArray items = doc.object().value("items").toArray();
    QJsonArray styles = doc.object().value("styles").toArray();

    progress->setMaximum(items.size() + styles.size());
    int step = 0;

    QJsonArray updatedStyles;
    for (const QJsonValue &sVal : styles)
    {
        if (progress->wasCanceled())
            break;
        progress->setValue(step++);
        QString css = sVal.toString();
        int end = 0;
        while (end >= 0 && !progress->wasCanceled())
        {
            int start = end;
            QString u = parseCssBgImageUrl(css, start, end);
            if (start >= 0 && !u.isEmpty())
            {
                QString res = savePageElement(QString(), QString(), u);
                if (!res.isEmpty())
                {
                    QString replacement = "url('" + res + "')";
                    css.replace(start, end - start, replacement);
                    end = start + replacement.length();
                }
            }
        }
        updatedStyles.append(css);
    }

    QJsonObject attrReplacements;
    for (const QJsonValue &itVal : items)
    {
        if (progress->wasCanceled())
            break;
        progress->setValue(step++);
        QJsonObject obj = itVal.toObject();
        QString tag = obj.value("tag").toString();
        QString attr = obj.value("attr").toString();
        QString val = obj.value("val").toString();
        QString key = tag + "|" + attr + "|" + val;
        if (!attrReplacements.contains(key))
        {
            QString res = savePageElement(tag, attr, val);
            if (!res.isEmpty())
            {
                if (attr == "style")
                {
                    int s = 0, e = 0;
                    parseCssBgImageUrl(val, s, e);
                    if (s >= 0)
                    {
                        QString newStyle = val;
                        newStyle.replace(s, e - s, "url('" + res + "')");
                        attrReplacements.insert(key, newStyle);
                    }
                }
                else
                {
                    attrReplacements.insert(key, res);
                }
            }
        }
    }

    // 2. Применяем замены на клоне документа в JS и получаем итоговый HTML
    QJsonObject payload;
    payload.insert("replacements", attrReplacements);
    payload.insert("styles", updatedStyles);
    QString payloadJson = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QString applyJs = QString(R"JS(
        (function(data) {
            var clone = document.documentElement.cloneNode(true);
            var styleEls = clone.querySelectorAll('style');
            for (var j = 0; j < styleEls.length && j < data.styles.length; j++) {
                styleEls[j].textContent = data.styles[j];
            }
            var selectors = [
                ['img', 'src'], ['script', 'src'], ['link', 'href'],
                ['a', 'href'], ['a', 'style'], ['div', 'style'],
                ['span', 'style'], ['iframe', 'src']
            ];
            selectors.forEach(function(pair) {
                var els = clone.querySelectorAll(pair[0]);
                for (var i = 0; i < els.length; i++) {
                    var val = els[i].getAttribute(pair[1]);
                    var key = pair[0] + '|' + pair[1] + '|' + val;
                    if (val && data.replacements[key]) {
                        els[i].setAttribute(pair[1], data.replacements[key]);
                    }
                }
            });
            return '<!DOCTYPE html>\n' + clone.outerHTML;
        })(%1)
    )JS").arg(payloadJson);

    QString finalHtml = webpage->evalJsSync(applyJs).toString();

    save_page->switchRoot();
    saveReplyToDisk(QUrl(Meow::excludeUndesiredSymbols(webpage->currentPageTitle()) + ".html"),
                    finalHtml.toUtf8(), "text/html");
    save_page->reset();

    QMetaObject::invokeMethod(webpage, "newWindowLink", Qt::QueuedConnection,
                              Q_ARG(QUrl, QUrl::fromLocalFile(last_file_name)));

    progress->hide();
    progress->deleteLater();
    webpage->setEnabled(true);
}
