#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookieJar>

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);
    ~DownloadManager();
    QNetworkReply * currentReply() const;
    void setCookieJar(QNetworkCookieJar * jar);
    void abort();
signals:
    void downloadProgress(qint64, qint64);
    void downloadFinished(QNetworkReply *); // only direct connect, don't delete it!
public slots:
    void downloadRequest(const QNetworkRequest & request);
private slots:
    void replyFinished(QNetworkReply *);
    void updownProgress(qint64 processed, qint64 total);
private:
    QNetworkAccessManager * manager;
    QNetworkReply * current_reply;
};

#endif // DOWNLOADMANAGER_H
