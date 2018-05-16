#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWidget>
#include <QUrl>
#include <QtWebKitWidgets/QWebHitTestResult>
#include <QtWebKitWidgets/QWebView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtWebKitWidgets/QWebInspector>
#include <QCompleter>

#include "settingspage.h"

namespace Ui {
class WebPage;
}

class FileManager;
class DownloadManager;
class MainWindow;
class WebPage : public QWidget
{
    Q_OBJECT

public:
    explicit WebPage(SettingsPage::Privacy _privacy = SettingsPage::UsualPrivacy, QWidget *parent = 0);
    ~WebPage();
    QWebView * view() const;
    QWebPage * page() const;
    SettingsPage::Privacy privacyMode() const;
    bool isToolWindow() const;
    bool isHiddenWindow() const;
    WebPage * createToolWindow(QUrl url, QString title = "", Qt::ConnectionType conn = Qt::QueuedConnection) const;
    WebPage * createHiddenWindow(QUrl url, Qt::ConnectionType conn = Qt::QueuedConnection) const;

    QUrl currentUrl() const { return current_url; }
    QString currentAltText() const { return current_alt_text; }
    QString currentPageTitle() const { return current_page_title; }
    QPoint currentPos() const { return current_pos; }

    void setMainWindow(const MainWindow * _mw);
public slots:
    void downloadRequest(const QNetworkRequest & request);
    void runScript();
    void inspectCurrentElement();
    void load(const QUrl & url);
    void savePage();
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
    void on_actionSearchImage_triggered();
    void on_actionTranslateSelected_triggered();

    void customContextMenuRequested(const QPoint & pos);

    void actionOpenLink();
    void actionOpenLinkInNewWindow();
    void actionOpenFrameInNewWindow();
    void actionOpenImageInNewWindow();
    void actionDownloadLinkToDisk();
    void actionDownloadImageToDisk();

    void downloadFinished(QNetworkReply *);
    void initialLayoutCompleted();
    void unsupportedContent(QNetworkReply * reply);
    void redirected(const QUrl &url);
    void featurePermissionRequested(QWebFrame* frame, QWebPage::Feature feature);
    void frameCreated(QWebFrame * frame);

    void updownProgress(qint64 processed, qint64 total);
private:
    Ui::WebPage *ui;
    QUrl current_url;
    QPoint current_pos;
    QString current_page_title;
    QString current_alt_text;
    QString last_file_name;
    SettingsPage::Privacy privacy;
    QString script;
    QWebInspector * inspector;
    QCompleter * completer;
    bool tool_window;
    bool temporary;
    QUrl requested_url;
    DownloadManager * dmanager;
    FileManager * fmanager;
    MainWindow * mw;
    void drawMeowCat();
    void setIncognito();
    void applyIcon();
    void applyTitle();
    QWebHitTestResult hitTest(QPoint) const;
    void runUserScriptRecursive(QWebFrame *);
    void updateCompleter(QString, QString);
protected:
    void showEvent(QShowEvent *);
signals:
    void newWindowLink(QUrl);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);
    void showNotifier(QString, int timeout = 5000);
    void downloadProgress(qint64, qint64);
    void downloadFinished();
};

#endif // WEBPAGE_H
