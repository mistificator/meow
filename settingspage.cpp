#include "settingspage.h"
#include "ui_settingspage.h"
#include <QDir>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtWebKit/QWebSettings>
#include <QUrl>
#include <QFileDialog>

#if QT_VERSION >= 0x050000
    #include <QStandardPaths>
#endif

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
    ui->defaultUseragentString->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
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

QSqlDatabase SettingsPage::database()
{
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    if (!db.isOpen())
    {
        db.setDatabaseName(tempDir() + "settings.sqlite");
        if (db.open())
        {
            databaseInit(db);
        }
    }
    return db;
}

void SettingsPage::databaseInit(QSqlDatabase & db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE GeneralSettings (Namespace TEXT, Field TEXT, Value TEXT)");
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

bool SettingsPage::setValue(QString field, QString value, QString _namespace)
{
    if (!database().isOpen())
    {
        qDebug() << "database not open";
        return false;
    }
    QSqlQuery q(database());
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
        qDebug() << "sql error" << q.lastError().text();
    }
    return q.lastError().type() == QSqlError::NoError;
}


QStringList SettingsPage::values(QString field, QString _namespace)
{
    if (!database().isOpen())
    {
        qDebug() << "database not open";
        return QStringList();
    }
    QSqlQuery q(database());
    if (!q.exec(QString("SELECT Value FROM GeneralSettings WHERE Field = \"%1\" AND Namespace = \"%2\"").arg(field, _namespace)))
    {
        qDebug() << "sql error" << q.lastError().text();
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
    return engines.at(index);
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
//        ui->searchEngine->setItemIcon(i, QWebSettings::iconForUrl(QUrl(url).adjusted(QUrl::RemovePath))); // don't work
        list << url;
    }
    return list;
}

void SettingsPage::on_searchEngine_currentIndexChanged(int index)
{
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(index);
}

void SettingsPage::on_searchEngine_editTextChanged(const QString &arg1)
{
}

void SettingsPage::on_searchRemove_clicked()
{
    ui->searchEngine->removeItem(ui->searchEngine->currentIndex());
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(ui->searchEngine->currentIndex());
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
