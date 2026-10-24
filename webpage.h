#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWidget>
#include <QUrl>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineDownloadRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCompleter>

#include "settingspage.h"

namespace Ui {
class WebPage;
}

class FileManager;
class DownloadManager;
class MainWindow;
class SharedCookieJar;

class WebPage : public QWidget
{
    Q_OBJECT

public:
    explicit WebPage(SettingsPage::Privacy _privacy = SettingsPage::UsualPrivacy, QWidget *parent = nullptr);
    ~WebPage() override;

    QWebEngineView *view() const;
    QWebEnginePage *page() const;
    SettingsPage::Privacy privacyMode() const;
    bool isToolWindow() const;
    bool isHiddenWindow() const;
    WebPage *createToolWindow(QUrl url, QString title = "", Qt::ConnectionType conn = Qt::QueuedConnection) const;
    WebPage *createHiddenWindow(QUrl url, Qt::ConnectionType conn = Qt::QueuedConnection) const;
    QUrl currentUrl() const { return current_url; }
    QString currentAltText() const { return current_alt_text; }
    QString currentPageTitle() const { return current_page_title; }
    QPoint currentPos() const { return current_pos; }
    void setMainWindow(const MainWindow *_mw);

    // Синхронное получение HTML или выполнение JS (для FileManager)
    QString fetchHtmlSync() const;
    QVariant evalJsSync(const QString &jsSource) const;

public slots:
    void downloadRequest(const QNetworkRequest &request);
    void runScript();
    void inspectCurrentElement();
    void load(const QUrl &url);
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
    void customContextMenuRequested(const QPoint &pos);
    void actionOpenLink();
    void actionOpenLinkInNewWindow();
    void actionOpenImageInNewWindow();
    void actionDownloadLinkToDisk();
    void actionDownloadImageToDisk();
    void networkDownloadFinished(QNetworkReply *);
    void featurePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);
    void updownProgress(qint64 processed, qint64 total);
    void on_webAddress_customContextMenuRequested(const QPoint &pos);
    void handleProfileDownload(QWebEngineDownloadRequest *download);

private:
    Ui::WebPage *ui;
    QUrl current_url;
    QPoint current_pos;
    QString current_page_title;
    QString current_alt_text;
    SettingsPage::Privacy privacy;
    QString script;
    QWebEngineView *inspector_view;
    QCompleter *completer;
    bool tool_window;
    bool temporary;
    QUrl requested_url;
    DownloadManager *dmanager;
    FileManager *fmanager;
    SharedCookieJar *cookie_jar;
    MainWindow *mw;

    static QWebEngineProfile *sharedProfile(SettingsPage::Privacy p);

    void drawMeowCat();
    void setIncognito();
    void applyIcon();
    void applyTitle();
    QString queryElementAltAt(const QPoint &pos) const;
    void runUserScript();
    void updateCompleter(QString phrase, QString desc);

protected:
    void showEvent(QShowEvent *) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void newWindowLink(QUrl);
    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void showNotifier(QString, int timeout = 5000);
    void downloadProgress(qint64, qint64);
    void downloadFinished(QNetworkReply *reply);
};

#endif // WEBPAGE_H
