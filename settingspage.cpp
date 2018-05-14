#include "settingspage.h"
#include "ui_settingspage.h"
#include <QDir>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtWebKit/QWebSettings>
#include <QUrl>
#include <QFileDialog>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QDesktopServices>

#if QT_VERSION >= 0x050000
    #include <QStandardPaths>
#endif

#include "custom_classes.h"

int SettingsPage::loaded_scripts_count = 0;
QString SettingsPage::user_script;

SettingsPage::SettingsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsPage)
{
    qRegisterMetaType<SettingsPage::ProxyType>("SettingsPage::ProxyType");
    qRegisterMetaType<SettingsPage::Privacy>("SettingsPage::Privacy");

    ui->setupUi(this);

    StartPage start_page = startPage();
    ui->pageEmpty->setChecked(start_page == PageEmpty);
    ui->pageLast->setChecked(start_page == PageLast);
    ui->pageHome->setChecked(start_page == PageHome);
    ui->pageAddress->setText(homePage());

    ProxyType proxy_type = proxyType();
    ui->proxyDisabled->setChecked(proxy_type == ProxyDisabled);
    ui->proxySystem->setChecked(proxy_type == ProxySystem);
    ui->proxyEnabled->setChecked(proxy_type == ProxySpecified);
    ui->proxyAdress->setText(proxyAddress());

    const int engine_index = currentSearchEngine();
    ui->searchEngine->addItems(searchEngines());
    ui->searchEngine->setCurrentIndex(engine_index);
    ui->searchRemove->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

    const int image_engine_index = currentImageSearchEngine();
    ui->imageSearchEngine->addItems(imageSearchEngines());
    ui->imageSearchEngine->setCurrentIndex(image_engine_index);
    ui->imageSearchRemove->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

    SavePolicy save_policy = savePolicy();
    ui->saveAsk->setChecked(save_policy == SaveAsk);
    ui->saveSilent->setChecked(save_policy == SaveSilent);
    ui->savePath->setText(savePath());
    ui->browseButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->writeMetadata->setChecked(writeMetadataEnabed());

    Privacy privacy = privacyPolicy();
    ui->privacyDefaultUsual->setChecked(privacy == UsualPrivacy);
    ui->privacyDefaultIncognito->setChecked(privacy == IncognitoPrivacy);
    ui->useragentString->setText(useragentString());
    ui->useragentString->home(true);
    ui->useragentString->deselect();
    ui->defaultUseragentString->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
    ui->reloadScripts->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    ui->openScriptsFolder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->clearHistory->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

    updateScriptsLoaded();
}

SettingsPage::~SettingsPage()
{
    delete ui;
}

QString SettingsPage::tempDir()
{
    QString temp_dir = QDir::tempPath() + "/meow/";
    if (!QDir().exists(temp_dir))
    {
        QDir().mkpath(temp_dir);
        qDebug() << "temp dir" << temp_dir;
    }
    return temp_dir;
}

QString SettingsPage::appDataDir()
{
    QString app_dir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).front();
    if (!QDir().exists(app_dir))
    {
        QDir().mkpath(app_dir);
        qDebug() << "app dir" << app_dir;
    }
    return app_dir;
}

QString SettingsPage::userScriptsDir()
{
    QString script_dir = appDataDir() + "/userscripts";
    if (!QDir().exists(script_dir))
    {
        QDir().mkpath(script_dir);
    }
    return script_dir;
}

class JSCheckWebPage: public QWebPage
{
public:
    JSCheckWebPage(QObject * _parent = 0): QWebPage(_parent)
    {
        clearMessage();
    }
    void clearMessage()
    {
        message.clear();
        source.clear();
        line = -1;
    }
protected:
    virtual void javaScriptConsoleMessage(const QString& _message, int _lineNumber, const QString& _sourceID)
    {
        message = _message;
        line = _lineNumber;
        source = _sourceID;
        qDebug() << message << line << source;
    }
public:
    QString message, source;
    int line;
};

void SettingsPage::loadScripts()
{
    QStringList scripts;
    JSCheckWebPage page;
    loaded_scripts_count = 0;
    foreach (QString filename, QDir(userScriptsDir()).entryList(QStringList() << "*.js", QDir::Files))
    {
        page.clearMessage();
        page.mainFrame()->setHtml("<html><head></head><body></body></html>");
        QFile js_file(userScriptsDir() + "/" + filename);
        if (js_file.open(QFile::ReadOnly))
        {
            QTextStream stream(& js_file);
            QString script = stream.readAll();
            js_file.close();
            page.mainFrame()->evaluateJavaScript(script);
            if (!page.message.contains("error", Qt::CaseInsensitive))
            {
                script = "// " + QFileInfo(filename).fileName() + "\r\n" + script;
                loaded_scripts_count++;
                qDebug() << filename << " loaded";
            }
            else
            {
                script = "// " + QFileInfo(filename).fileName() + " has error, " + page.message + " at line " + QString::number(page.line) + "\r\n";
                qDebug() << script;
            }
            scripts << script;
        }
    }
    user_script = scripts.join("\r\n\r\n");
}

QString SettingsPage::userScript()
{
    return user_script;
}

int SettingsPage::userScriptsLoaded()
{
    return loaded_scripts_count;
}

QSqlDatabase SettingsPage::databaseSettings()
{
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "settings");
    if (!db.isOpen())
    {
        db.setDatabaseName(appDataDir() + "/settings.sqlite");
        if (db.open())
        {
            databaseSettingsInit(db);
        }
    }
    return db;
}

void SettingsPage::databaseSettingsInit(QSqlDatabase & db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE GeneralSettings (Namespace TEXT DEFAULT \"\", Field TEXT DEFAULT \"\", Value TEXT DEFAULT \"\")");
    if (q.lastError().type() != QSqlError::NoError)
    {
        return;
    }
    setStartPage(PageLast);
    setHomePage("https://duckduckgo.com");
    setProxyType(ProxySystem);
    setSearchEngines(QStringList() <<
                     "duckduckgo.com/?q=%1" <<
                     "google.com/search?q=%1" <<
                     "yandex.com/search/?text=%1");
    setImageSearchEngines(QStringList() <<
                     "google.com/searchbyimage?image_url=%1" <<
                     "yandex.com/images/search?rpt=imageview&url=%1");

    QString download_location;
#if QT_VERSION >= 0x050000
    QStringList list = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    if (!list.isEmpty())
    {
        download_location = list.front();
    }
    else
#endif
    {
        download_location = QDir::homePath();
    }
    setCurrentSavePath(download_location);
    setSavePath(download_location + "/meow");
    setWriteMetadataEnabled(true);
    setUseragentString(QString());
    setPrivacyPolicy(UsualPrivacy);
}

QSqlDatabase SettingsPage::databaseHistory()
{
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history");
    if (!db.isOpen())
    {
        db.setDatabaseName(appDataDir() + "/history.sqlite");
        if (db.open())
        {
            databaseHistoryInit(db);
        }
    }
    return db;
}

void SettingsPage::databaseHistoryInit(QSqlDatabase & db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE CompleterHistory (Id INTEGER PRIMARY KEY AUTOINCREMENT, Namespace TEXT DEFAULT \"\", Phrase TEXT DEFAULT \"\", Description TEXT DEFAULT \"\", Frequency INTEGER DEFAULT \"0\")");
    q.exec("CREATE TABLE DownloadsHistory (Id INTEGER PRIMARY KEY AUTOINCREMENT, DateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, Namespace TEXT DEFAULT \"\", Description TEXT DEFAULT \"\", Title TEXT DEFAULT \"\", File TEXT DEFAULT \"\", Size INTEGER DEFAULT \"0\", Page TEXT DEFAULT \"\", Url TEXT DEFAULT \"\")");
    if (q.lastError().type() != QSqlError::NoError)
    {
        return;
    }
}

bool SettingsPage::setValue(QString field, QString value, QString _namespace)
{
    if (!databaseSettings().isOpen())
    {
        qDebug() << "database not open";
        return false;
    }
    QSqlQuery q(databaseSettings());
    if (!values(field).isEmpty())
    {
        if (q.exec(QString("UPDATE GeneralSettings SET Value = \"%2\" WHERE Field = \"%1\" AND Namespace = \"%3\"").arg(field, value, _namespace)))
        {
            return q.lastError().type() == QSqlError::NoError;
        }
    }
    q.exec(QString("INSERT INTO GeneralSettings (Field, Value, Namespace) VALUES (\"%1\", \"%2\", \"%3\")").arg(field, value, _namespace));
    if (q.lastError().type() != QSqlError::NoError)
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
    }
    return q.lastError().type() == QSqlError::NoError;
}


QStringList SettingsPage::values(QString field, QString _namespace)
{
    if (!databaseSettings().isOpen())
    {
        qDebug() << "database not open";
        return QStringList();
    }
    QSqlQuery q(databaseSettings());
    if (!q.exec(QString("SELECT Value FROM GeneralSettings WHERE Field = \"%1\" AND Namespace = \"%2\"").arg(field, _namespace)))
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return QStringList();
    }
    QStringList list;
    while (q.next())
    {
        list << q.value(0).toString();
    }
    return list;
}

QString SettingsPage::value(QString field, QString defaultValue, QString _namespace)
{
    QStringList list = values(field, _namespace);
    if (list.isEmpty())
    {
        return defaultValue;
    }
    return list.front();
}

bool SettingsPage::setStartPage(StartPage page)
{
    return setValue("StartPage", QString::number(page));
}

SettingsPage::StartPage SettingsPage::startPage()
{
    return (SettingsPage::StartPage)value("StartPage").toInt();
}

bool SettingsPage::setHomePage(QString address)
{
    return setValue("HomePage", address);
}

QString SettingsPage::homePage()
{
    return value("HomePage");
}

bool SettingsPage::setLastPages(QStringList addresses)
{
    return setValue("LastPages", addresses.join("\n"));
}

QStringList SettingsPage::lastPages()
{
    return value("LastPages").split("\n");
}

bool SettingsPage::setProxyType(ProxyType proxy)
{
    return setValue("ProxyType", QString::number(proxy));
}

SettingsPage::ProxyType SettingsPage::proxyType()
{
    return (ProxyType)value("ProxyType").toInt();
}

bool SettingsPage::setProxyAddress(QString address)
{
    return setValue("ProxyAddress", address);
}

QString SettingsPage::proxyAddress()
{
    return value("ProxyAddress");
}

bool SettingsPage::setSearchEngines(QStringList list)
{
    return setValue("SearchEngines", list.join("\n"));
}

QStringList SettingsPage::searchEngines()
{
    return value("SearchEngines").split("\n");
}

bool SettingsPage::setCurrentSearchEngine(int index)
{
    return setValue("SearchEngineCurrent", QString::number(index));
}

int SettingsPage::currentSearchEngine()
{
    const int index = value("SearchEngineCurrent").toInt();
    if (index < 0 || index >= searchEngines().count())
    {
        return -1;
    }
    return index;
}

QString SettingsPage::currentSearchEngineUrl()
{
    const int index = value("SearchEngineCurrent").toInt();
    const QStringList engines = searchEngines();
    if (index < 0 || index >= engines.count())
    {
        return QString();
    }
    QString url(engines.at(index));
    if (QUrl(url).scheme().isEmpty())
    {
        url = "https://" + url;
    }
    return url;
}

bool SettingsPage::setImageSearchEngines(QStringList list)
{
    return setValue("ImageSearchEngines", list.join("\n"));
}

QStringList SettingsPage::imageSearchEngines()
{
    return value("ImageSearchEngines").split("\n");
}

bool SettingsPage::setCurrentImageSearchEngine(int index)
{
    return setValue("ImageSearchEngineCurrent", QString::number(index));
}

int SettingsPage::currentImageSearchEngine()
{
    const int index = value("ImageSearchEngineCurrent").toInt();
    if (index < 0 || index >= imageSearchEngines().count())
    {
        return -1;
    }
    return index;
}

QString SettingsPage::currentImageSearchEngineUrl()
{
    const int index = value("ImageSearchEngineCurrent").toInt();
    const QStringList engines = imageSearchEngines();
    if (index < 0 || index >= engines.count())
    {
        return QString();
    }
    QString url(engines.at(index));
    if (QUrl(url).scheme().isEmpty())
    {
        url = "https://" + url;
    }
    return url;
}

bool SettingsPage::setSavePolicy(SavePolicy policy)
{
    return setValue("SavePolicy", QString::number(policy));
}

SettingsPage::SavePolicy SettingsPage::savePolicy()
{
    return (SavePolicy)value("SavePolicy").toInt();
}

bool SettingsPage::setSavePath(QString path)
{
    return setValue("SavePath", path);
}

QString SettingsPage::savePath()
{
    return value("SavePath");
}

bool SettingsPage::setCurrentSavePath(QString path)
{
    return setValue("CurrentSavePath", path);
}

QString SettingsPage::currentSavePath()
{
    return value("CurrentSavePath");
}

bool SettingsPage::setWriteMetadataEnabled(bool state)
{
    return setValue("WriteMetadata", QString::number(state));
}

bool SettingsPage::writeMetadataEnabed()
{
    return value("WriteMetadata").toInt() != 0;
}

bool SettingsPage::setPrivacyPolicy(Privacy privacy)
{
    return setValue("PrivacyPolicy", QString::number(privacy));
}

SettingsPage::Privacy SettingsPage::privacyPolicy()
{
    return (Privacy)value("PrivacyPolicy").toInt();
}

bool SettingsPage::setUseragentString(QString str)
{
    return setValue("Useragent", str);
}

QString SettingsPage::useragentString()
{
    return value("Useragent");
}

bool SettingsPage::setItWasSuccessExit(bool state)
{
    return setValue("FatalExit", QString::number(state == 0));
}

bool SettingsPage::itWasSuccessExit()
{
    return value("FatalExit").toInt() == 0;
}

QSqlTableModel * SettingsPage::completerHistoryModel()
{
    static QSqlTableModel * model = 0;
    if (model == 0)
    {
        model = new HintSqlTableModel(0, databaseHistory());
        model->setTable("CompleterHistory");
        model->setSort(model->fieldIndex("Frequency"), Qt::DescendingOrder);
    }
    return model;
}

bool SettingsPage::addCompleterPhrase(QString phrase, QString desc, QString _namespace)
{
    QSqlTableModel * model = completerHistoryModel();
    QSqlQuery q(databaseHistory());
    if (q.exec(QString("SELECT Id FROM %1 WHERE Phrase = \"%2\"").arg(model->tableName(), phrase)) && q.next())
    {
        if (q.exec(QString("UPDATE %1 SET Frequency = Frequency + 1 WHERE Id = \"%2\"")
                      .arg(model->tableName(), q.value(0).toString())))
        {
            return model->select();
        }
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return false;
    }
    if (!q.exec(QString("INSERT INTO %1 (Namespace, Phrase, Description) VALUES (\"%2\", \"%3\", \"%4\")")
                .arg(model->tableName(), _namespace, phrase, desc)))
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return false;
    }
    return model->select();
}

bool SettingsPage::clearCompleterHistory()
{
    QSqlTableModel * model = completerHistoryModel();
    QSqlQuery q(databaseHistory());
    if (!q.exec(QString("DELETE FROM %1").arg(model->tableName())))
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return false;
    }
    return model->select();
}

QSqlTableModel * SettingsPage::downloadsHistoryModel()
{
    static QSqlTableModel * model = 0;
    if (model == 0)
    {
        model = new HintSqlTableModel(0, databaseHistory());
        model->setTable("DownloadsHistory");
        model->setSort(model->fieldIndex("DateTime"), Qt::DescendingOrder);
    }
    return model;
}

bool SettingsPage::addDownloadEntry(QString file, QString url, QString page, qint64 size, QString title, QString description, QDateTime datetime, QString _namespace)
{
    QSqlTableModel * model = downloadsHistoryModel();
    QSqlQuery q(databaseHistory());
    if (!q.exec(QString("INSERT INTO %1 (DateTime, Namespace, File, Url, Page, Size, Title, Description) VALUES (\"%2\", \"%3\", \"%4\", \"%5\", \"%6\", \"%7\", \"%8\", \"%9\")")
                .arg(model->tableName(), datetime.toString("yyyy-MM-ddThh:mm:ss.zzz"), _namespace, file, url, page, QString::number(size), title, description)))
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return false;
    }
    return model->select();

}

bool SettingsPage::clearDownloadsHistory()
{
    QSqlTableModel * model = downloadsHistoryModel();
    QSqlQuery q(databaseHistory());
    if (!q.exec(QString("DELETE FROM %1").arg(model->tableName())))
    {
        qDebug() << __FUNCTION__ << q.lastError().text() << q.lastQuery();
        return false;
    }
    return model->select();
}

QString SettingsPage::cookieJar()
{
    return appDataDir() + "/cookies.";
}

bool SettingsPage::clearCookies()
{
    SharedCookieJar::clear();
    return QFile::remove(cookieJar());
}

void SettingsPage::on_pageEmpty_toggled(bool checked)
{
    if (checked)
    {
        setStartPage(PageEmpty);
    }
}

void SettingsPage::on_pageLast_toggled(bool checked)
{
    if (checked)
    {
        setStartPage(PageLast);
    }
}

void SettingsPage::on_pageHome_toggled(bool checked)
{
    if (checked)
    {
        setStartPage(PageHome);
    }
}

void SettingsPage::on_pageAddress_editingFinished()
{
    setHomePage(ui->pageAddress->text());
}

void SettingsPage::on_proxyDisabled_toggled(bool checked)
{
    if (checked)
    {
        setProxyType(ProxyDisabled);
        emit proxyChanged(ProxyDisabled);
    }
}

void SettingsPage::on_proxySystem_toggled(bool checked)
{
    if (checked)
    {
        setProxyType(ProxySystem);
        emit proxyChanged(ProxySystem);
    }
}

void SettingsPage::on_proxyEnabled_toggled(bool checked)
{
    if (checked)
    {
        setProxyType(ProxySpecified);
        emit proxyChanged(ProxySpecified);
    }
}

QStringList SettingsPage::updateSearchEngines()
{
    QStringList list;
    for (int i = 0; i < ui->searchEngine->count(); i++)
    {
        QString url =  ui->searchEngine->itemText(i);
        ui->searchEngine->setItemIcon(i, QWebSettings::iconForUrl(QUrl("https://" + QUrl(url).host()))); // doesn't work
        list << url;
    }
    return list;
}

QStringList SettingsPage::updateImageSearchEngines()
{
    QStringList list;
    for (int i = 0; i < ui->imageSearchEngine->count(); i++)
    {
        QString url =  ui->imageSearchEngine->itemText(i);
        ui->searchEngine->setItemIcon(i, QWebSettings::iconForUrl(QUrl("https://" + QUrl(url).host()))); // doesn't work
        list << url;
    }
    return list;
}

void SettingsPage::on_searchEngine_currentIndexChanged(int index)
{
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(index);
}

void SettingsPage::on_searchRemove_clicked()
{
    ui->searchEngine->removeItem(ui->searchEngine->currentIndex());
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(ui->searchEngine->currentIndex());
}

void SettingsPage::on_imageSearchEngine_currentIndexChanged(int index)
{
    setImageSearchEngines(updateImageSearchEngines());
    setCurrentImageSearchEngine(index);
}

void SettingsPage::on_imageSearchRemove_clicked()
{
    ui->imageSearchEngine->removeItem(ui->imageSearchEngine->currentIndex());
    setImageSearchEngines(updateImageSearchEngines());
    setCurrentImageSearchEngine(ui->imageSearchEngine->currentIndex());
}

void SettingsPage::on_saveAsk_toggled(bool checked)
{
    if (checked)
    {
        setSavePolicy(SaveAsk);
    }
}

void SettingsPage::on_saveSilent_toggled(bool checked)
{
    if (checked)
    {
        setSavePolicy(SaveSilent);
    }
}

void SettingsPage::on_savePath_editingFinished()
{
    setSavePath(ui->savePath->text());
}

void SettingsPage::on_browseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(topLevelWidget(), "Select folder for downloads");
    if (dir.isEmpty())
    {
        return;
    }
    ui->savePath->setText(dir);
    setSavePath(dir);
}

void SettingsPage::on_writeMetadata_toggled(bool checked)
{
    setWriteMetadataEnabled(checked);
}

void SettingsPage::on_privacyDefaultUsual_toggled(bool checked)
{
    if (checked)
    {
        setPrivacyPolicy(UsualPrivacy);
        emit privacyChanged(UsualPrivacy);
    }
}

void SettingsPage::on_privacyDefaultIncognito_toggled(bool checked)
{
    if (checked)
    {
        setPrivacyPolicy(IncognitoPrivacy);
        emit privacyChanged(IncognitoPrivacy);
    }
}

void SettingsPage::on_useragentString_editingFinished()
{
    setUseragentString(ui->useragentString->text());
}

void SettingsPage::on_defaultUseragentString_clicked()
{
    setUseragentString(QString());
}

void SettingsPage::on_reloadScripts_clicked()
{
    loadScripts();
    updateScriptsLoaded();
}

void SettingsPage::updateScriptsLoaded()
{
    ui->scriptCount->setText(
                userScriptsLoaded() == 0 ? "no custom scripts" :
                userScriptsLoaded() == 1 ? "1 custom script" :
                QString::number(userScriptsLoaded()) + " custom scripts"
                            );
}

void SettingsPage::on_openScriptsFolder_clicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(userScriptsDir())));
}

void SettingsPage::on_clearHistory_clicked()
{
    clearCompleterHistory();
    clearDownloadsHistory();
    clearCookies();
}

