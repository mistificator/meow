#ifndef CUSTOM_CLASSES_H
#define CUSTOM_CLASSES_H

#include <QNetworkRequest>
#include <QUrl>
#include <QCompleter>
#include <QtWebKitWidgets/QWebPage>
#include <QStyledItemDelegate>
#include <QtSql/QSqlTableModel>

#include "settingspage.h"

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

class CustomNetworkRequest: public QNetworkRequest
{
public:
    CustomNetworkRequest(const QUrl &url, KnownHeaders header = (KnownHeaders)-1, QVariant value = QVariant()): QNetworkRequest(url)
    {
        setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
        setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        if (header != (KnownHeaders)-1)
        {
            setHeader(header, value);
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

#endif // CUSTOM_CLASSES_H
