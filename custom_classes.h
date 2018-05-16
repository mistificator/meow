#ifndef CUSTOM_CLASSES_H
#define CUSTOM_CLASSES_H

#include <QNetworkRequest>
#include <QUrl>
#include <QCompleter>
#include <QtWebKitWidgets/QWebPage>
#include <QStyledItemDelegate>
#include <QtSql/QSqlTableModel>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QFile>
#include <QDebug>
#include <QtWebKitWidgets/QWebFrame>
#include <QApplication>
#include <QFileInfo>

#include "settingspage.h"
#include "webpage.h"
#include "mainwindow.h"

class CustomWebPage : public QWebPage
{
public:
    CustomWebPage(QObject * _parent = 0): QWebPage(_parent), mw(0)
    {
//        setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    }
    void setMainWindow(const MainWindow * _mw)
    {
        mw = (MainWindow *)_mw;
    }
protected:    
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
    {
//        qDebug() << "acceptNavigationRequest" << request.url().host();
        return QWebPage::acceptNavigationRequest(frame, request, type);
    }
    virtual QObject * createPlugin(const QString & classid, const QUrl & url, const QStringList & paramNames, const QStringList & paramValues)
    {
        qDebug() << "create plugin" << classid << url;
        return QWebPage::createPlugin(classid, url, paramNames, paramValues);
    }
    virtual QWebPage * createWindow(WebWindowType type)
    {
        if (mw == 0 || view() == 0)
        {
            return QWebPage::createWindow(type);
        }
        qDebug() << "createWindow";
        mw->newWindow(dynamic_cast<WebPage *>(view()->parentWidget()) ? dynamic_cast<WebPage *>(view()->parentWidget())->privacyMode() : SettingsPage::UsualPrivacy);
        return mw->lastPage()->page();
    }
    virtual void javaScriptAlert(QWebFrame * frame, const QString & msg)
    {
        qDebug() << "alert" << msg;
        return QWebPage::javaScriptAlert(frame, msg);
    }
    virtual bool javaScriptConfirm(QWebFrame * frame, const QString & msg)
    {
        qDebug() << "confirm" << msg;
        return QWebPage::javaScriptConfirm(frame, msg);
    }
    virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID)
    {
//        qDebug() << "log" << message << lineNumber << sourceID;
        QWebPage::javaScriptConsoleMessage(message, lineNumber, sourceID);
    }
    virtual bool javaScriptPrompt(QWebFrame * frame, const QString & msg, const QString & defaultValue, QString * result)
    {
        qDebug() << "prompt" << msg;
        return QWebPage::javaScriptPrompt(frame, msg, defaultValue, result);
    }
    virtual QString userAgentForUrl(const QUrl &) const
    {
        QString str = SettingsPage::useragentString();
        if (str.isEmpty())
        {
            SettingsPage::setUseragentString(str = QWebPage::userAgentForUrl(QUrl("https://example.com")));
        }
        return str;
    }
private:
    MainWindow * mw;
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

class CustomNetworkRequest: public QNetworkRequest
{
public:
    struct Headers
    {
        QByteArray header;
        QByteArray value;
    };
    CustomNetworkRequest(const QUrl &url, QList<Headers> headers = QList<Headers>()): QNetworkRequest(url)
    {
        setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
        setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        foreach (const Headers & header, headers)
        {
            setRawHeader(header.header, header.value);
        }
    }
};

class HintSqlTableModel: public QSqlTableModel
{
public:
    HintSqlTableModel(QObject * _parent = 0, QSqlDatabase db = QSqlDatabase()): QSqlTableModel(_parent, db) {}
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const
    {
        if (role == Qt::ToolTipRole)
        {
            return QSqlTableModel::data(idx, Qt::DisplayRole);
        }
        return QSqlTableModel::data(idx, role);
    }
};

class DownloadsUrlColumnDelegate: public QStyledItemDelegate
{
public:
    DownloadsUrlColumnDelegate(QObject * _parent = 0): QStyledItemDelegate(_parent) {}
protected:
    QString displayText(const QVariant & value, const QLocale &) const
    {
        return QFileInfo(value.toString()).fileName();
    }
};

class DownloadsTimeColumnDelegate: public QStyledItemDelegate
{
public:
    DownloadsTimeColumnDelegate(QObject * _parent = 0): QStyledItemDelegate(_parent) {}
protected:
    QString displayText(const QVariant & value, const QLocale &) const
    {
        return QDateTime::fromString(value.toString(), "yyyy-MM-ddThh:mm:ss.zzz").toString("hh:mm, dd.MM");
    }
};

class DownloadsSizeColumnDelegate: public QStyledItemDelegate
{
public:
    DownloadsSizeColumnDelegate(QObject * _parent = 0): QStyledItemDelegate(_parent) {}
protected:
    QString displayText(const QVariant & value, const QLocale &) const
    {
        qint64 size = value.toLongLong();
        if (size < 0.9 * 1024)
        {
            return QString::number(size) + " b";
        }
        size /= 1024;
        if (size < 0.9 * 1024)
        {
            return QString::number(size) + " Kb";
        }
        size /= 1024;
        if (size < 0.9 * 1024)
        {
            return QString::number(size) + " Mb";
        }
        size /= 1024;
        return QString::number(size) + " Gb";
    }
};

// idea from https://stackoverflow.com/questions/13971787/how-do-i-save-cookies-with-qt
class SharedCookieJar: public QNetworkCookieJar
{
public:
    SharedCookieJar(SettingsPage::Privacy _privacy, QObject *parent = 0) : QNetworkCookieJar(parent), privacy(_privacy)
    {
        if (privacy == SettingsPage::UsualPrivacy && * refCount(privacy) == 0)
        {
            qDebug() << "read cookies from" << SettingsPage::cookieJar();
            QFile file(SettingsPage::cookieJar());
            if (file.open(QFile::ReadOnly))
            {
                JarOpener::setAllCookies(sharedJar(privacy), QNetworkCookie::parseCookies(file.readAll()));
                file.close();
            }
        }
        (* refCount(privacy))++;
//        setAllCookies(JarOpener::allCookies(sharedJar()));
    }
    virtual ~SharedCookieJar()
    {
        (* refCount(privacy))--;
        if (* refCount(privacy) == 0)
        {
            if (privacy == SettingsPage::UsualPrivacy)
            {
                flush();
            }
            delete refCount(privacy);
            delete sharedJar(privacy);
            refCount(privacy) = 0;
            sharedJar(privacy) = 0;
        }
    }
    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const
    {
        return sharedJar(privacy)->cookiesForUrl(url);
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
    {
        return sharedJar(privacy)->setCookiesFromUrl(cookieList, url);
    }

    virtual bool deleteCookie(const QNetworkCookie &cookie)
    {
        return sharedJar(privacy)->deleteCookie(cookie);
    }
    virtual bool insertCookie(const QNetworkCookie &cookie)
    {
        return sharedJar(privacy)->insertCookie(cookie);
    }
    virtual bool updateCookie(const QNetworkCookie &cookie)
    {
        return sharedJar(privacy)->updateCookie(cookie);
    }
    static void clear()
    {
        JarOpener::setAllCookies(sharedJar(SettingsPage::UsualPrivacy), QList<QNetworkCookie>());
        JarOpener::setAllCookies(sharedJar(SettingsPage::IncognitoPrivacy), QList<QNetworkCookie>());
    }
    static void flush()
    {
        QByteArray data;
        foreach (QNetworkCookie cookie, JarOpener::allCookies(sharedJar(SettingsPage::UsualPrivacy)))
        {
            if (!cookie.isSessionCookie())
            {
                data.append(cookie.toRawForm());
                data.append("\n");
            }
        }
        qDebug() << "write cookies to" << SettingsPage::cookieJar();
        QFile file(SettingsPage::cookieJar());
        if (file.open(QFile::WriteOnly))
        {
            file.write(data);
            file.close();
        }
    }
protected:
    virtual bool validateCookie(const QNetworkCookie &cookie, const QUrl &url) const
    {
        return JarOpener::validateCookie(sharedJar(privacy), cookie, url);
    }
private:
    SettingsPage::Privacy privacy;
    static int * & refCount(SettingsPage::Privacy privacy)
    {
        static int * ref_count[2] = {0, 0};
        if (ref_count[privacy] == 0)
        {
            ref_count[privacy] = new int;
            * ref_count[privacy] = 0;
        }
        return ref_count[privacy];
    }
    static QNetworkCookieJar * & sharedJar(SettingsPage::Privacy privacy)
    {
        static QNetworkCookieJar * jar[2] = {0, 0};
        if (jar[privacy] == 0)
        {
            jar[privacy] = new QNetworkCookieJar();
        }
        return jar[privacy];
    }
    class JarOpener: public QNetworkCookieJar
    {
    public:
        static QList<QNetworkCookie> allCookies(QNetworkCookieJar * jar)
        {
            return ((JarOpener *)jar)->QNetworkCookieJar::allCookies();
        }
        static bool validateCookie(QNetworkCookieJar * jar, const QNetworkCookie &cookie, const QUrl &url)
        {
            return ((JarOpener *)jar)->QNetworkCookieJar::validateCookie(cookie, url);
        }
        static void setAllCookies(QNetworkCookieJar * jar, const QList<QNetworkCookie> &cookieList)
        {
            ((JarOpener *)jar)->QNetworkCookieJar::setAllCookies(cookieList);
        }
    };
};

#endif // CUSTOM_CLASSES_H
