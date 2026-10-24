#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>
#include <QDateTime>

namespace Ui
{
    class SettingsPage;
}

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = 0);
    ~SettingsPage();

    static QString tempDir();
    static QString appDataDir();
    static QString userScriptsDir();
    static QString userScript();
    static void loadScripts();
    static int userScriptsLoaded();

    enum StartPage {PageEmpty = 0, PageLast = 1, PageHome = 2};
    static bool setStartPage(StartPage page);
    static StartPage startPage();
    static bool setHomePage(QString address);
    static QString homePage();
    static bool setLastPages(QStringList addresses);
    static QStringList lastPages();

    enum ProxyType {ProxyDisabled = 0, ProxySystem = 1, ProxySpecified = 2};
    static bool setProxyType(ProxyType proxy);
    static ProxyType proxyType();
    static bool setProxyAddress(QString address);
    static QString proxyAddress();

    static bool setSearchEngines(QStringList);
    static QStringList searchEngines();
    static bool setCurrentSearchEngine(int);
    static int currentSearchEngine();
    static QString currentSearchEngineUrl();

    static bool setImageSearchEngines(QStringList);
    static QStringList imageSearchEngines();
    static bool setCurrentImageSearchEngine(int);
    static int currentImageSearchEngine();
    static QString currentImageSearchEngineUrl();

    static bool setTranslateEngines(QStringList);
    static QStringList translateEngines();
    static bool setCurrentTranslateEngine(int);
    static int currentTranslateEngine();
    static QString currentTranslateEngineUrl();

    enum SavePolicy {SaveAsk = 0, SaveSilent = 1};
    static bool setSavePolicy(SavePolicy);
    static SavePolicy savePolicy();
    static bool setSavePath(QString);
    static QString savePath();
    static bool setCurrentSavePath(QString);
    static QString currentSavePath();
    static bool setWriteMetadataEnabled(bool);
    static bool writeMetadataEnabed();

    enum Privacy {UsualPrivacy = 0, IncognitoPrivacy = 1};
    static bool setPrivacyPolicy(Privacy);
    static Privacy privacyPolicy();
    static bool setUseragentString(QString); // https://deviceatlas.com/blog/list-of-user-agent-strings
    static QString useragentString();

    static bool setItWasSuccessExit(bool);
    static bool itWasSuccessExit();

    static bool setValue(QString field, QString value, QString _namespace = QString());
    static QStringList values(QString field, QString _namespace = QString());
    static QString value(QString field, QString defaultValue = QString(), QString _namespace = QString());

    static QSqlDatabase databaseSettings();

    static QSqlTableModel * completerHistoryModel();
    static bool addCompleterPhrase(QString phrase, QString desc, QString _namespace = QString());
    static bool clearCompleterHistory();

    static QSqlTableModel * downloadsHistoryModel();
    static bool addDownloadEntry(QString file, QString url, QString page, qint64 size, QString title, QString description, QDateTime datetime = QDateTime::currentDateTime(), QString _namespace = QString());
    static bool clearDownloadsHistory();

    static QSqlDatabase databaseHistory();

    static QString cookieJar();
    static bool clearCookies();
signals:
    void proxyChanged(SettingsPage::ProxyType);
    void privacyChanged(SettingsPage::Privacy);
    void showNotifier(QString, int timeout = 5000);
private slots:
    void on_pageEmpty_toggled(bool checked);
    void on_pageLast_toggled(bool checked);
    void on_pageHome_toggled(bool checked);
    void on_pageAddress_editingFinished();
    void on_proxyDisabled_toggled(bool checked);
    void on_proxySystem_toggled(bool checked);
    void on_proxyEnabled_toggled(bool checked);
    void on_searchEngine_currentIndexChanged(int index);
    void on_searchRemove_clicked();
    void on_imageSearchEngine_currentIndexChanged(int index);
    void on_imageSearchRemove_clicked();
    void on_saveAsk_toggled(bool checked);
    void on_saveSilent_toggled(bool checked);
    void on_savePath_editingFinished();
    void on_browseButton_clicked();
    void on_writeMetadata_toggled(bool checked);
    void on_privacyDefaultUsual_toggled(bool checked);
    void on_privacyDefaultIncognito_toggled(bool checked);
    void on_useragentString_editingFinished();
    void on_defaultUseragentString_clicked();
    void on_reloadScripts_clicked();
    void on_openScriptsFolder_clicked();
    void on_clearHistory_clicked();
    void on_translateRemove_clicked();
    void on_translateEngine_currentIndexChanged(int index);
    void on_proxyAdress_editingFinished();
private:
    bool m_initializing = false;
    Ui::SettingsPage *ui;
    QStringList updateSearchEngines();
    QStringList updateImageSearchEngines();
    QStringList updateTranslateEngines();
    void updateScriptsLoaded();
    static QString user_script;
    static int loaded_scripts_count;
    static void databaseSettingsInit(QSqlDatabase & db);
    static void databaseHistoryInit(QSqlDatabase & db);
};

#endif // SETTINGSPAGE_H
