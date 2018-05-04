#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWidget>
#include <QUrl>
#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebView>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "filemetadata.h"
#include "settingspage.h"

namespace Ui {
class WebPage;
}

class WebPage : public QWidget
{
    Q_OBJECT

public:
    enum Privacy {UsualPrivacy = SettingsPage::UsualPrivacy, IncognitoPrivacy = SettingsPage::IncognitoPrivacy};
    explicit WebPage(Privacy _privacy = UsualPrivacy, QWidget *parent = 0);
    ~WebPage();
    QWebView * view() const;
    QWebPage * page() const;
    bool isIncognito() const;
private slots:
    void on_actionLoadPage_triggered();
    void on_actionReloadPage_triggered();
    void on_actionBack_triggered();
    void on_actionForward_triggered();
    void on_webView_loadFinished(bool);
    void on_webView_titleChanged(const QString &);
    void on_webView_urlChanged(const QUrl &);
    void on_webView_iconChanged();
    void on_webView_loadStarted();
    void customContextMenuRequested(const QPoint & pos);

    void actionOpenLink();
    void actionOpenLinkInNewWindow();
    void actionOpenFrameInNewWindow();
    void actionOpenImageInNewWindow();
    void actionDownloadLinkToDisk();
    void actionDownloadImageToDisk();

    void replyFinished(QNetworkReply *);
    void linkClicked(QUrl);
    void initialLayoutCompleted();
    void downloadRequested(const QNetworkRequest & request);
    void unsupportedContent(QNetworkReply * reply);
    void featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature);

    void updownProgress(qint64 processed, qint64 total);
private:
    Ui::WebPage *ui;
    QUrl current_url;
    QPoint current_pos;
    QString current_page_title;
    QString current_alt_text;
    QNetworkAccessManager * manager;
    FileMetadata fmeta;
    Privacy privacy;
    void setIncognito();
    void applyIcon();
    void applyTitle();
    QWebHitTestResult hitTest(QPoint) const;
    static QString adjustFilename(QString filename, const QByteArray & data);
    void saveReplyToDisk(QNetworkReply * reply);
    void modifyExif(QString filename, QUrl file_url, QUrl page_url, QStringList tags);
    QRegExp undesiredSymbols(QString str = "\"&,\\:?/") const;
protected:
    void showEvent(QShowEvent *);
signals:
    void newWindowLink(QUrl);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);
    void showNotifier(QString, int timeout = 5000);
};

#endif // WEBPAGE_H
