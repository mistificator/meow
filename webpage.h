#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWidget>
#include <QUrl>
#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebView>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class WebPage;
}

class WebPage : public QWidget
{
    Q_OBJECT

public:
    explicit WebPage(QWidget *parent = 0);
    ~WebPage();
    QWebView * view() const;
    QWebPage * page() const;
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
//    void actionDownloadLinkToDisk();
//    void actionDownloadImageToDisk();

    void replyFinished(QNetworkReply *);
    void linkClicked(QUrl);
    void initialLayoutCompleted();
    void downloadRequested(const QNetworkRequest & request);
    void unsupportedContent(QNetworkReply * reply);
    void featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature);
private:
    Ui::WebPage *ui;
    QUrl current_url;
    QPoint current_pos;
    QNetworkAccessManager * manager;
    QString download_location;
    void applyIcon();
    void applyTitle();
    QWebHitTestResult hitTest(QPoint) const;
    static QString adjustFilename(QString filename, const QByteArray & data);
    void saveReplyToDisk(QNetworkReply * reply);
protected:
    void showEvent(QShowEvent *);
signals:
    void newWindowLink(QUrl);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);
    void showNotifier(QString, int timeout = 5000);
};

#endif // WEBPAGE_H
