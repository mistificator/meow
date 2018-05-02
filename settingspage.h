#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QtSql/QSqlDatabase>

namespace Ui {
class SettingsPage;
}

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = 0);
    ~SettingsPage();

    static QString tempDir();

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
signals:
    void proxyChanged(SettingsPage::ProxyType);
private slots:
    void on_pageEmpty_toggled(bool checked);
    void on_pageLast_toggled(bool checked);
    void on_pageHome_toggled(bool checked);
    void on_pageAddress_editingFinished();
    void on_proxyDisabled_toggled(bool checked);
    void on_proxySystem_toggled(bool checked);
    void on_proxyEnabled_toggled(bool checked);
    void on_searchEngine_currentIndexChanged(int index);
    void on_searchEngine_editTextChanged(const QString &arg1);
    void on_searchRemove_clicked();
    void on_saveAsk_toggled(bool checked);
    void on_saveSilent_toggled(bool checked);
    void on_savePath_editingFinished();
    void on_browseButton_clicked();
    void on_writeMetadata_toggled(bool checked);
private:
    Ui::SettingsPage *ui;
    QStringList updateSearchEngines();
    static QSqlDatabase database();
    static void databaseInit(QSqlDatabase & db);
    static bool setValue(QString field, QString value, QString _namespace = QString());
    static QStringList values(QString field, QString _namespace = QString());
    static QString value(QString field, QString defaultValue = QString(), QString _namespace = QString());
};

#endif // SETTINGSPAGE_H
