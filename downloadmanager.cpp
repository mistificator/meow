#include "downloadmanager.h"

DownloadManager::DownloadManager(QObject *parent) : QObject(parent), current_reply(0)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));
}

DownloadManager::~DownloadManager()
{
    if (current_reply)
    {
        abort();
        qDebug() << "deleting previous reply";
        current_reply->deleteLater();
    }
}

void DownloadManager::abort()
{
    if (current_reply && (current_reply->isRunning() || !current_reply->isFinished()))
    {
        qDebug() << "aborting previous reply";
        current_reply->abort();
    }
}

QNetworkReply * DownloadManager::currentReply() const
{
    return current_reply;
}

void DownloadManager::setCookieJar(QNetworkCookieJar * jar)
{
    manager->setCookieJar(jar);
}

void DownloadManager::downloadRequest(const QNetworkRequest & request)
{
    if (current_reply)
    {
        abort();
        qDebug() << "deleting previous reply";
        current_reply->deleteLater();
    }
    qDebug() << "request" << request.url();
    current_reply = manager->get(request);
    connect(current_reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(downloadProgress(qint64, qint64)));
    connect(current_reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(updownProgress(qint64,qint64)));
    qDebug() << "downloading started";

}

void DownloadManager::replyFinished(QNetworkReply * reply)
{
    if (current_reply != reply)
    {
        qDebug() << "lost reply received!!!" << current_reply << reply;
    }
    else
    {
        emit downloadFinished(reply);
    }
}

void DownloadManager::updownProgress(qint64 processed, qint64 total)
{
    qDebug() << "updown" << processed << "/" << total;
}
