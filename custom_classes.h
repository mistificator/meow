#ifndef CUSTOM_CLASSES_H
#define CUSTOM_CLASSES_H

#include <QNetworkRequest>
#include <QUrl>
#include <QCompleter>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QStyledItemDelegate>
#include <QtSql/QSqlTableModel>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QFileInfo>
#include <QDateTime>

#include "settingspage.h"
#include "webpage.h"
#include "mainwindow.h"

// SharedCookieJar объявляется до CustomWebPage для синхронизации с профилем
class SharedCookieJar : public QNetworkCookieJar
{
public:
    SharedCookieJar(SettingsPage::Privacy _privacy, QObject *parent = nullptr)
        : QNetworkCookieJar(parent), privacy(_privacy)
    {
        if (privacy == SettingsPage::UsualPrivacy && *refCount(privacy) == 0)
        {
            QFile file(SettingsPage::cookieJar());
            if (file.open(QFile::ReadOnly))
            {
                JarOpener::setAllCookies(sharedJar(privacy), QNetworkCookie::parseCookies(file.readAll()));
                file.close();
            }
        }
        (*refCount(privacy))++;
    }

    ~SharedCookieJar() override
    {
        (*refCount(privacy))--;
        if (*refCount(privacy) <= 0)
        {
            if (privacy == SettingsPage::UsualPrivacy)
            {
                flush();
            }
            delete refCount(privacy);
            delete sharedJar(privacy);
            refCount(privacy) = nullptr;
            sharedJar(privacy) = nullptr;
        }
    }

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override
    {
        return sharedJar(privacy)->cookiesForUrl(url);
    }

    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) override
    {
        return sharedJar(privacy)->setCookiesFromUrl(cookieList, url);
    }

    bool deleteCookie(const QNetworkCookie &cookie) override
    {
        return sharedJar(privacy)->deleteCookie(cookie);
    }

    bool insertCookie(const QNetworkCookie &cookie) override
    {
        return sharedJar(privacy)->insertCookie(cookie);
    }

    bool updateCookie(const QNetworkCookie &cookie) override
    {
        return sharedJar(privacy)->updateCookie(cookie);
    }

    static QList<QNetworkCookie> getAllSharedCookies(SettingsPage::Privacy p)
    {
        return JarOpener::allCookies(sharedJar(p));
    }

    static void clear()
    {
        JarOpener::setAllCookies(sharedJar(SettingsPage::UsualPrivacy), QList<QNetworkCookie>());
        JarOpener::setAllCookies(sharedJar(SettingsPage::IncognitoPrivacy), QList<QNetworkCookie>());
    }

    static void flush()
    {
        QByteArray data;
        const auto cookies = JarOpener::allCookies(sharedJar(SettingsPage::UsualPrivacy));
        for (const QNetworkCookie &cookie : cookies)
        {
            if (!cookie.isSessionCookie())
            {
                data.append(cookie.toRawForm());
                data.append("\n");
            }
        }
        QFile file(SettingsPage::cookieJar());
        if (file.open(QFile::WriteOnly))
        {
            file.write(data);
            file.close();
        }
    }

protected:
    bool validateCookie(const QNetworkCookie &cookie, const QUrl &url) const override
    {
        return JarOpener::validateCookie(sharedJar(privacy), cookie, url);
    }

private:
    SettingsPage::Privacy privacy;

    static int *&refCount(SettingsPage::Privacy privacy)
    {
        static int *ref_count[2] = {nullptr, nullptr};
        if (ref_count[privacy] == nullptr)
        {
            ref_count[privacy] = new int(0);
        }
        return ref_count[privacy];
    }

    static QNetworkCookieJar *&sharedJar(SettingsPage::Privacy privacy)
    {
        static QNetworkCookieJar *jar[2] = {nullptr, nullptr};
        if (jar[privacy] == nullptr)
        {
            jar[privacy] = new QNetworkCookieJar();
        }
        return jar[privacy];
    }

    class JarOpener : public QNetworkCookieJar
    {
    public:
        static QList<QNetworkCookie> allCookies(QNetworkCookieJar *jar)
        {
            return static_cast<JarOpener *>(jar)->QNetworkCookieJar::allCookies();
        }
        static bool validateCookie(QNetworkCookieJar *jar, const QNetworkCookie &cookie, const QUrl &url)
        {
            return static_cast<JarOpener *>(jar)->QNetworkCookieJar::validateCookie(cookie, url);
        }
        static void setAllCookies(QNetworkCookieJar *jar, const QList<QNetworkCookie> &cookieList)
        {
            static_cast<JarOpener *>(jar)->QNetworkCookieJar::setAllCookies(cookieList);
        }
    };
};

class CustomWebPage : public QWebEnginePage
{
public:
    CustomWebPage(QWebEngineProfile *profile, QObject *_parent = nullptr)
        : QWebEnginePage(profile, _parent), mw(nullptr), parent_webpage(nullptr)
    {
        updateUserAgent();
    }

    void setMainWindow(const MainWindow *_mw)
    {
        mw = const_cast<MainWindow *>(_mw);
    }

    void setWebPageWidget(WebPage *wp)
    {
        parent_webpage = wp;
    }

    void updateUserAgent()
    {
        QString str = SettingsPage::useragentString();
        if (str.isEmpty())
        {
            str = profile()->httpUserAgent();
            SettingsPage::setUseragentString(str);
        }
        else
        {
            profile()->setHttpUserAgent(str);
        }
    }

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        if (url.isLocalFile() && !url.host().isEmpty())
        {
            qDebug() << "skip request" << url;
            return false;
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

    QWebEnginePage *createWindow(WebWindowType type) override
    {
        if (mw == nullptr || parent_webpage == nullptr)
        {
            return QWebEnginePage::createWindow(type);
        }
        qDebug() << "createWindow";
        mw->newWindow(parent_webpage->privacyMode());
        return mw->lastPage() ? mw->lastPage()->page() : nullptr;
    }

    void javaScriptAlert(const QUrl &securityOrigin, const QString &msg) override
    {
        qDebug() << "alert" << msg;
        QWebEnginePage::javaScriptAlert(securityOrigin, msg);
    }

    bool javaScriptConfirm(const QUrl &securityOrigin, const QString &msg) override
    {
        qDebug() << "confirm" << msg;
        return QWebEnginePage::javaScriptConfirm(securityOrigin, msg);
    }

    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override
    {
        QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
    }

    bool javaScriptPrompt(const QUrl &securityOrigin,
                          const QString &msg,
                          const QString &defaultValue,
                          QString *result) override
    {
        qDebug() << "prompt" << msg;
        return QWebEnginePage::javaScriptPrompt(securityOrigin, msg, defaultValue, result);
    }

private:
    MainWindow *mw;
    WebPage *parent_webpage;
};

class CompleterColumnDelegate : public QStyledItemDelegate
{
public:
    CompleterColumnDelegate(QObject *_parent = nullptr) : QStyledItemDelegate(_parent) {}

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
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

class UrlCompleter : public QCompleter
{
public:
    UrlCompleter(QObject *_parent) : QCompleter(_parent) {}
    QString pathFromIndex(const QModelIndex &index) const override
    {
        QSqlTableModel *model = SettingsPage::completerHistoryModel();
        QString desc = model->data(model->index(index.row(), model->fieldIndex("Description"))).toString();
        if (!desc.isEmpty())
        {
            return desc;
        }
        return index.data().toString();
    }
};

class CustomNetworkRequest : public QNetworkRequest
{
public:
    struct Headers
    {
        QByteArray header;
        QByteArray value;
    };
    CustomNetworkRequest(const QUrl &url, const QList<Headers> &headers = QList<Headers>())
        : QNetworkRequest(url)
    {
        setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
        setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        for (const Headers &header : headers)
        {
            setRawHeader(header.header, header.value);
        }
    }
};

class HintSqlTableModel : public QSqlTableModel
{
public:
    HintSqlTableModel(QObject *_parent = nullptr, QSqlDatabase db = QSqlDatabase())
        : QSqlTableModel(_parent, db) {}
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::ToolTipRole)
        {
            return QSqlTableModel::data(idx, Qt::DisplayRole);
        }
        return QSqlTableModel::data(idx, role);
    }
};

class DownloadsUrlColumnDelegate : public QStyledItemDelegate
{
public:
    DownloadsUrlColumnDelegate(QObject *_parent = nullptr) : QStyledItemDelegate(_parent) {}

protected:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        return QFileInfo(value.toString()).fileName();
    }
};

class DownloadsTimeColumnDelegate : public QStyledItemDelegate
{
public:
    DownloadsTimeColumnDelegate(QObject *_parent = nullptr) : QStyledItemDelegate(_parent) {}

protected:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        return QDateTime::fromString(value.toString(), "yyyy-MM-ddThh:mm:ss.zzz").toString("hh:mm, dd.MM");
    }
};

class DownloadsSizeColumnDelegate : public QStyledItemDelegate
{
public:
    DownloadsSizeColumnDelegate(QObject *_parent = nullptr) : QStyledItemDelegate(_parent) {}

protected:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        qint64 size = value.toLongLong();
        if (size < 0.9 * 1024)
            return QString::number(size) + " b";
        size /= 1024;
        if (size < 0.9 * 1024)
            return QString::number(size) + " Kb";
        size /= 1024;
        if (size < 0.9 * 1024)
            return QString::number(size) + " Mb";
        size /= 1024;
        return QString::number(size) + " Gb";
    }
};

#endif // CUSTOM_CLASSES_H
