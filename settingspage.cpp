#include "settingspage.h"
#include "ui_settingspage.h"

#include <QDir>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QJSEngine>
#include <QJSValue>
#include <QLocale>
#include <QTextStream>
#include <QLineEdit>

#include "custom_classes.h"
#include "utils.h"
#include "mainwindow.h"

int SettingsPage::loaded_scripts_count = 0;
QString SettingsPage::user_script;

// Гарантирует, что QString() (isNull() == true) не превратится в SQL NULL при bindValue()
static QString nonNull(const QString &s)
{
    return s.isNull() ? QStringLiteral("") : s;
}

SettingsPage::SettingsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsPage),
    m_initializing(true)
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

    const int translate_engine_index = currentTranslateEngine();
    ui->translateEngine->addItems(translateEngines());
    ui->translateEngine->setCurrentIndex(translate_engine_index);
    ui->translateRemove->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

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

    // Сохранение при редактировании текста в редактируемых QComboBox
    if (ui->searchEngine->lineEdit())
    {
        connect(ui->searchEngine->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            if (!m_initializing)
            {
                ui->searchEngine->setItemText(ui->searchEngine->currentIndex(), ui->searchEngine->currentText());
                setSearchEngines(updateSearchEngines());
            }
        });
    }
    if (ui->imageSearchEngine->lineEdit())
    {
        connect(ui->imageSearchEngine->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            if (!m_initializing)
            {
                ui->imageSearchEngine->setItemText(ui->imageSearchEngine->currentIndex(), ui->imageSearchEngine->currentText());
                setImageSearchEngines(updateImageSearchEngines());
            }
        });
    }
    if (ui->translateEngine->lineEdit())
    {
        connect(ui->translateEngine->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            if (!m_initializing)
            {
                ui->translateEngine->setItemText(ui->translateEngine->currentIndex(), ui->translateEngine->currentText());
                setTranslateEngines(updateTranslateEngines());
            }
        });
    }

    m_initializing = false;
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
    }
    return temp_dir;
}

QString SettingsPage::appDataDir()
{
    QString app_dir = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).front();
    if (!QDir().exists(app_dir))
    {
        QDir().mkpath(app_dir);
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

void SettingsPage::loadScripts()
{
    QStringList scripts;
    QJSEngine engine;
    loaded_scripts_count = 0;

    const QStringList jsFiles = QDir(userScriptsDir()).entryList(QStringList() << "*.js", QDir::Files);
    for (const QString &filename : jsFiles)
    {
        QFile js_file(userScriptsDir() + "/" + filename);
        if (js_file.open(QFile::ReadOnly))
        {
            QTextStream stream(&js_file);
            QString script = stream.readAll();
            js_file.close();

            QJSValue check = engine.evaluate("(function() {\n" + script + "\n})");
            if (!check.isError())
            {
                script = "// " + QFileInfo(filename).fileName() + "\r\n" + script;
                loaded_scripts_count++;
                qDebug() << filename << " loaded";
            }
            else
            {
                script = "// " + QFileInfo(filename).fileName() + " has error, " +
                         check.toString() + " at line " +
                         QString::number(check.property("lineNumber").toInt()) + "\r\n";
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
    static bool initialized = false;
    QSqlDatabase db = QSqlDatabase::contains("settings")
                          ? QSqlDatabase::database("settings")
                          : QSqlDatabase::addDatabase("QSQLITE", "settings");
    if (!db.isOpen())
    {
        db.setDatabaseName(appDataDir() + "/settings.sqlite");
        db.open();
    }
    if (db.isOpen() && !initialized)
    {
        initialized = true;
        databaseSettingsInit(db);
    }
    return db;
}

void SettingsPage::databaseSettingsInit(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS GeneralSettings (Namespace TEXT DEFAULT '', Field TEXT DEFAULT '', Value TEXT DEFAULT '')");

    // Очищаем поврежденные записи с Namespace IS NULL от предыдущих запусков
    q.exec("DELETE FROM GeneralSettings WHERE Namespace IS NULL");

    // Проверяем, инициализирована ли база базовыми настройками
    q.exec("SELECT COUNT(*) FROM GeneralSettings WHERE Field = 'HomePage'");
    if (q.next() && q.value(0).toInt() > 0)
    {
        return;
    }

    setStartPage(PageLast);
    setHomePage("https://duckduckgo.com");
    setProxyType(ProxySystem);
    setSearchEngines(QStringList()
                     << "duckduckgo.com/?q=%1"
                     << "google.com/search?q=%1"
                     << "yandex.com/search/?text=%1");
    setCurrentSearchEngine(0);

    setImageSearchEngines(QStringList()
                          << "www.google.com/searchbyimage?image_url=%1"
                          << "yandex.com/images/search?rpt=imageview&url=%1");
    setCurrentImageSearchEngine(0);

    setTranslateEngines(QStringList()
                        << "translate.google.com/?hl=#auto/" + QLocale::system().bcp47Name().left(2) + "/%1");
    setCurrentTranslateEngine(0);

    QString download_location;
    QStringList list = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    download_location = !list.isEmpty() ? list.front() : QDir::homePath();

    setSavePolicy(SaveAsk);
    setCurrentSavePath(download_location);
    setSavePath(download_location + "/meow");
    setWriteMetadataEnabled(true);
    setUseragentString(QStringLiteral(""));
    setPrivacyPolicy(UsualPrivacy);
}

QSqlDatabase SettingsPage::databaseHistory()
{
    static bool initialized = false;
    QSqlDatabase db = QSqlDatabase::contains("history")
                          ? QSqlDatabase::database("history")
                          : QSqlDatabase::addDatabase("QSQLITE", "history");
    if (!db.isOpen())
    {
        db.setDatabaseName(appDataDir() + "/history.sqlite");
        db.open();
    }
    if (db.isOpen() && !initialized)
    {
        initialized = true;
        databaseHistoryInit(db);
    }
    return db;
}

void SettingsPage::databaseHistoryInit(QSqlDatabase &db)
{
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS CompleterHistory (Id INTEGER PRIMARY KEY AUTOINCREMENT, Namespace TEXT DEFAULT '', Phrase TEXT DEFAULT '', Description TEXT DEFAULT '', Frequency INTEGER DEFAULT 0)");
    q.exec("CREATE TABLE IF NOT EXISTS DownloadsHistory (Id INTEGER PRIMARY KEY AUTOINCREMENT, DateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP, Namespace TEXT DEFAULT '', Description TEXT DEFAULT '', Title TEXT DEFAULT '', File TEXT DEFAULT '', Size INTEGER DEFAULT 0, Page TEXT DEFAULT '', Url TEXT DEFAULT '')");
}

bool SettingsPage::setValue(QString field, QString value, QString _namespace)
{
    QSqlDatabase db = databaseSettings();
    if (!db.isOpen())
    {
        qDebug() << "database not open";
        return false;
    }

    const QString f = nonNull(field);
    const QString v = nonNull(value);
    const QString ns = nonNull(_namespace);

    QSqlQuery check(db);
    check.prepare("SELECT COUNT(*) FROM GeneralSettings WHERE Field = :field AND Namespace = :ns");
    check.bindValue(":field", f);
    check.bindValue(":ns", ns);

    QSqlQuery q(db);
    if (check.exec() && check.next() && check.value(0).toInt() > 0)
    {
        q.prepare("UPDATE GeneralSettings SET Value = :val WHERE Field = :field AND Namespace = :ns");
        q.bindValue(":val", v);
        q.bindValue(":field", f);
        q.bindValue(":ns", ns);
        return q.exec();
    }

    q.prepare("INSERT INTO GeneralSettings (Field, Value, Namespace) VALUES (:field, :val, :ns)");
    q.bindValue(":field", f);
    q.bindValue(":val", v);
    q.bindValue(":ns", ns);
    if (!q.exec())
    {
        qDebug() << __FUNCTION__ << q.lastError().text();
        return false;
    }
    return true;
}

QStringList SettingsPage::values(QString field, QString _namespace)
{
    QSqlDatabase db = databaseSettings();
    if (!db.isOpen())
    {
        return QStringList();
    }

    QSqlQuery q(db);
    q.prepare("SELECT Value FROM GeneralSettings WHERE Field = :field AND Namespace = :ns");
    q.bindValue(":field", nonNull(field));
    q.bindValue(":ns", nonNull(_namespace));
    if (!q.exec())
    {
        qDebug() << __FUNCTION__ << q.lastError().text();
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
    return static_cast<SettingsPage::StartPage>(value("StartPage", QString::number(PageLast)).toInt());
}

bool SettingsPage::setHomePage(QString address)
{
    return setValue("HomePage", address);
}

QString SettingsPage::homePage()
{
    return value("HomePage", "https://duckduckgo.com");
}

bool SettingsPage::setLastPages(QStringList addresses)
{
    return setValue("LastPages", addresses.join("\n"));
}

QStringList SettingsPage::lastPages()
{
    return value("LastPages").split("\n", Qt::SkipEmptyParts);
}

bool SettingsPage::setProxyType(ProxyType proxy)
{
    return setValue("ProxyType", QString::number(proxy));
}

SettingsPage::ProxyType SettingsPage::proxyType()
{
    return static_cast<ProxyType>(value("ProxyType", QString::number(ProxySystem)).toInt());
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
    return value("SearchEngines").split("\n", Qt::SkipEmptyParts);
}

bool SettingsPage::setCurrentSearchEngine(int index)
{
    return setValue("SearchEngineCurrent", QString::number(index));
}

int SettingsPage::currentSearchEngine()
{
    const QStringList engines = searchEngines();
    if (engines.isEmpty())
        return -1;
    const int index = value("SearchEngineCurrent", "0").toInt();
    return (index < 0 || index >= engines.count()) ? 0 : index;
}

QString SettingsPage::currentSearchEngineUrl()
{
    const int index = currentSearchEngine();
    const QStringList engines = searchEngines();
    if (index < 0 || index >= engines.count())
        return QString();

    QString url(engines.at(index));
    if (QUrl(url).scheme().isEmpty())
        url = "https://" + url;
    if (!url.contains("%1"))
        url += "%1";
    return url;
}

bool SettingsPage::setImageSearchEngines(QStringList list)
{
    return setValue("ImageSearchEngines", list.join("\n"));
}

QStringList SettingsPage::imageSearchEngines()
{
    return value("ImageSearchEngines").split("\n", Qt::SkipEmptyParts);
}

bool SettingsPage::setCurrentImageSearchEngine(int index)
{
    return setValue("ImageSearchEngineCurrent", QString::number(index));
}

int SettingsPage::currentImageSearchEngine()
{
    const QStringList engines = imageSearchEngines();
    if (engines.isEmpty())
        return -1;
    const int index = value("ImageSearchEngineCurrent", "0").toInt();
    return (index < 0 || index >= engines.count()) ? 0 : index;
}

QString SettingsPage::currentImageSearchEngineUrl()
{
    const int index = currentImageSearchEngine();
    const QStringList engines = imageSearchEngines();
    if (index < 0 || index >= engines.count())
        return QString();

    QString url(engines.at(index));
    if (QUrl(url).scheme().isEmpty())
        url = "https://" + url;
    if (!url.contains("%1"))
        url += "%1";
    return url;
}

bool SettingsPage::setTranslateEngines(QStringList list)
{
    return setValue("TranslateEngines", list.join("\n"));
}

QStringList SettingsPage::translateEngines()
{
    return value("TranslateEngines").split("\n", Qt::SkipEmptyParts);
}

bool SettingsPage::setCurrentTranslateEngine(int index)
{
    return setValue("TranslateEngineCurrent", QString::number(index));
}

int SettingsPage::currentTranslateEngine()
{
    const QStringList engines = translateEngines();
    if (engines.isEmpty())
        return -1;
    const int index = value("TranslateEngineCurrent", "0").toInt();
    return (index < 0 || index >= engines.count()) ? 0 : index;
}

QString SettingsPage::currentTranslateEngineUrl()
{
    const int index = currentTranslateEngine();
    const QStringList engines = translateEngines();
    if (index < 0 || index >= engines.count())
        return QString();

    QString url(engines.at(index));
    if (QUrl(url).scheme().isEmpty())
        url = "https://" + url;
    if (!url.contains("%1"))
        url += "%1";
    return url;
}

bool SettingsPage::setSavePolicy(SavePolicy policy)
{
    return setValue("SavePolicy", QString::number(policy));
}

SettingsPage::SavePolicy SettingsPage::savePolicy()
{
    return static_cast<SavePolicy>(value("SavePolicy", QString::number(SaveAsk)).toInt());
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
    return setValue("WriteMetadata", QString::number(state ? 1 : 0));
}

bool SettingsPage::writeMetadataEnabed()
{
    return value("WriteMetadata", "1").toInt() != 0;
}

bool SettingsPage::setPrivacyPolicy(Privacy privacy)
{
    return setValue("PrivacyPolicy", QString::number(privacy));
}

SettingsPage::Privacy SettingsPage::privacyPolicy()
{
    return static_cast<Privacy>(value("PrivacyPolicy", QString::number(UsualPrivacy)).toInt());
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
    return setValue("FatalExit", QString::number(state ? 0 : 1));
}

bool SettingsPage::itWasSuccessExit()
{
    return value("FatalExit", "0").toInt() == 0;
}

QSqlTableModel *SettingsPage::completerHistoryModel()
{
    static QSqlTableModel *model = nullptr;
    if (model == nullptr)
    {
        model = new HintSqlTableModel(nullptr, databaseHistory());
        model->setTable("CompleterHistory");
        model->setSort(model->fieldIndex("Frequency"), Qt::DescendingOrder);
        model->select();
    }
    return model;
}

bool SettingsPage::addCompleterPhrase(QString phrase, QString desc, QString _namespace)
{
    QSqlTableModel *model = completerHistoryModel();
    QSqlQuery q(databaseHistory());
    q.prepare(QString("SELECT Id FROM %1 WHERE Phrase = :phrase").arg(model->tableName()));
    q.bindValue(":phrase", nonNull(phrase));
    if (q.exec() && q.next())
    {
        int id = q.value(0).toInt();
        QSqlQuery up(databaseHistory());
        up.prepare(QString("UPDATE %1 SET Frequency = Frequency + 1 WHERE Id = :id").arg(model->tableName()));
        up.bindValue(":id", id);
        if (up.exec())
            return model->select();
        return false;
    }

    q.prepare(QString("INSERT INTO %1 (Namespace, Phrase, Description, Frequency) VALUES (:ns, :phrase, :desc, 1)").arg(model->tableName()));
    q.bindValue(":ns", nonNull(_namespace));
    q.bindValue(":phrase", nonNull(phrase));
    q.bindValue(":desc", nonNull(desc));
    if (!q.exec())
        return false;

    return model->select();
}

bool SettingsPage::clearCompleterHistory()
{
    QSqlTableModel *model = completerHistoryModel();
    QSqlQuery q(databaseHistory());
    if (!q.exec(QString("DELETE FROM %1").arg(model->tableName())))
        return false;
    return model->select();
}

QSqlTableModel *SettingsPage::downloadsHistoryModel()
{
    static QSqlTableModel *model = nullptr;
    if (model == nullptr)
    {
        model = new HintSqlTableModel(nullptr, databaseHistory());
        model->setTable("DownloadsHistory");
        model->setSort(model->fieldIndex("DateTime"), Qt::DescendingOrder);
        model->select();
    }
    return model;
}

bool SettingsPage::addDownloadEntry(QString file, QString url, QString page, qint64 size, QString title, QString description, QDateTime datetime, QString _namespace)
{
    QSqlTableModel *model = downloadsHistoryModel();
    QSqlQuery q(databaseHistory());
    q.prepare(QString("INSERT INTO %1 (DateTime, Namespace, File, Url, Page, Size, Title, Description) "
                      "VALUES (:dt, :ns, :file, :url, :page, :size, :title, :desc)").arg(model->tableName()));
    q.bindValue(":dt", datetime.toString("yyyy-MM-ddThh:mm:ss.zzz"));
    q.bindValue(":ns", nonNull(_namespace));
    q.bindValue(":file", nonNull(file));
    q.bindValue(":url", nonNull(url));
    q.bindValue(":page", nonNull(page));
    q.bindValue(":size", size);
    q.bindValue(":title", nonNull(title));
    q.bindValue(":desc", nonNull(description));
    if (!q.exec())
        return false;
    return model->select();
}

bool SettingsPage::clearDownloadsHistory()
{
    QSqlTableModel *model = downloadsHistoryModel();
    QSqlQuery q(databaseHistory());
    if (!q.exec(QString("DELETE FROM %1").arg(model->tableName())))
        return false;
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
    if (!m_initializing && checked)
        setStartPage(PageEmpty);
}

void SettingsPage::on_pageLast_toggled(bool checked)
{
    if (!m_initializing && checked)
        setStartPage(PageLast);
}

void SettingsPage::on_pageHome_toggled(bool checked)
{
    if (!m_initializing && checked)
        setStartPage(PageHome);
}

void SettingsPage::on_pageAddress_editingFinished()
{
    if (!m_initializing)
        setHomePage(ui->pageAddress->text());
}

void SettingsPage::on_proxyDisabled_toggled(bool checked)
{
    if (!m_initializing && checked)
    {
        setProxyType(ProxyDisabled);
        emit proxyChanged(ProxyDisabled);
    }
}

void SettingsPage::on_proxySystem_toggled(bool checked)
{
    if (!m_initializing && checked)
    {
        setProxyType(ProxySystem);
        emit proxyChanged(ProxySystem);
    }
}

void SettingsPage::on_proxyEnabled_toggled(bool checked)
{
    if (!m_initializing && checked)
    {
        setProxyType(ProxySpecified);
        emit proxyChanged(ProxySpecified);
    }
}

void SettingsPage::on_proxyAdress_editingFinished()
{
    if (!m_initializing)
    {
        setProxyAddress(ui->proxyAdress->text());
        if (ui->proxyEnabled->isChecked())
        {
            emit proxyChanged(ProxySpecified);
        }
    }
}

QStringList SettingsPage::updateSearchEngines()
{
    QStringList list;
    for (int i = 0; i < ui->searchEngine->count(); i++)
        list << ui->searchEngine->itemText(i);
    return list;
}

QStringList SettingsPage::updateImageSearchEngines()
{
    QStringList list;
    for (int i = 0; i < ui->imageSearchEngine->count(); i++)
        list << ui->imageSearchEngine->itemText(i);
    return list;
}

QStringList SettingsPage::updateTranslateEngines()
{
    QStringList list;
    for (int i = 0; i < ui->translateEngine->count(); i++)
        list << ui->translateEngine->itemText(i);
    return list;
}

void SettingsPage::on_searchEngine_currentIndexChanged(int index)
{
    if (m_initializing)
        return;
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(index);
}

void SettingsPage::on_searchRemove_clicked()
{
    m_initializing = true;
    ui->searchEngine->removeItem(ui->searchEngine->currentIndex());
    m_initializing = false;
    setSearchEngines(updateSearchEngines());
    setCurrentSearchEngine(ui->searchEngine->currentIndex());
}

void SettingsPage::on_imageSearchEngine_currentIndexChanged(int index)
{
    if (m_initializing)
        return;
    setImageSearchEngines(updateImageSearchEngines());
    setCurrentImageSearchEngine(index);
}

void SettingsPage::on_imageSearchRemove_clicked()
{
    m_initializing = true;
    ui->imageSearchEngine->removeItem(ui->imageSearchEngine->currentIndex());
    m_initializing = false;
    setImageSearchEngines(updateImageSearchEngines());
    setCurrentImageSearchEngine(ui->imageSearchEngine->currentIndex());
}

void SettingsPage::on_translateEngine_currentIndexChanged(int index)
{
    if (m_initializing)
        return;
    setTranslateEngines(updateTranslateEngines());
    setCurrentTranslateEngine(index);
}

void SettingsPage::on_translateRemove_clicked()
{
    m_initializing = true;
    ui->translateEngine->removeItem(ui->translateEngine->currentIndex());
    m_initializing = false;
    setTranslateEngines(updateTranslateEngines());
    setCurrentTranslateEngine(ui->translateEngine->currentIndex());
}

void SettingsPage::on_saveAsk_toggled(bool checked)
{
    if (!m_initializing && checked)
        setSavePolicy(SaveAsk);
}

void SettingsPage::on_saveSilent_toggled(bool checked)
{
    if (!m_initializing && checked)
        setSavePolicy(SaveSilent);
}

void SettingsPage::on_savePath_editingFinished()
{
    if (!m_initializing)
        setSavePath(ui->savePath->text());
}

void SettingsPage::on_browseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(topLevelWidget(), "Select folder for downloads");
    if (dir.isEmpty())
        return;
    ui->savePath->setText(dir);
    setSavePath(dir);
}

void SettingsPage::on_writeMetadata_toggled(bool checked)
{
    if (!m_initializing)
        setWriteMetadataEnabled(checked);
}

void SettingsPage::on_privacyDefaultUsual_toggled(bool checked)
{
    if (!m_initializing && checked)
    {
        setPrivacyPolicy(UsualPrivacy);
        emit privacyChanged(UsualPrivacy);
    }
}

void SettingsPage::on_privacyDefaultIncognito_toggled(bool checked)
{
    if (!m_initializing && checked)
    {
        setPrivacyPolicy(IncognitoPrivacy);
        emit privacyChanged(IncognitoPrivacy);
    }
}

void SettingsPage::on_useragentString_editingFinished()
{
    if (!m_initializing)
        setUseragentString(ui->useragentString->text());
}

void SettingsPage::on_defaultUseragentString_clicked()
{
    ui->useragentString->clear();
    setUseragentString(QStringLiteral(""));
}

void SettingsPage::on_reloadScripts_clicked()
{
    loadScripts();
    updateScriptsLoaded();
    emit showNotifier("Scripts updated");
}

void SettingsPage::updateScriptsLoaded()
{
    ui->scriptCount->setText(
        userScriptsLoaded() == 0 ? "no custom scripts" :
            userScriptsLoaded() == 1 ? "1 custom script" :
            QString::number(userScriptsLoaded()) + " custom scripts");
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
    emit showNotifier("History cleared");
}
