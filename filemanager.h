#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include "webpage.h"
#include "filemetadata.h"

class WebPage;
class FileManager : public QObject
{
    Q_OBJECT
public:
    explicit FileManager(QObject *parent = 0);
    ~FileManager();
    void setWebPage(WebPage *);
    void saveReplyToDisk(QNetworkReply * reply);
    void saveReplyToDisk(const QUrl & url, const QByteArray & data, QString content_type);
    bool checkRedirect(QNetworkReply * reply) const;
    void setStateFrom(FileManager *);
signals:
    void showNotifier(QString, int timeout = 5000);
public slots:
    void savePage();
private:
    WebPage * webpage;
    QString last_file_name;
    FileMetadata fmeta;
    struct SavePageStruct;
    SavePageStruct * save_page;
    static QString adjustFilename(QString filename, QString content, const QByteArray & data);
    void modifyExif(QString filename, QUrl file_url, QUrl page_url, QStringList tags);
    QString parseCssBgImageUrl(const QString & css, int & start, int & end);
    QString savePageElement(QString tag, QString attr, QString value);
};

#endif // FILEMANAGER_H
