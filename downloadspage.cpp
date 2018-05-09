#include "downloadspage.h"
#include "ui_downloadspage.h"
#include "settingspage.h"

#include <QDesktopServices>
#include <QStyledItemDelegate>
#include <QFileInfo>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>

#include "custom_classes.h"

DownloadsPage::DownloadsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadsPage)
{
    ui->setupUi(this);

    ui->openFile->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->openFolder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->openPage->setIcon(style()->standardIcon(QStyle::SP_CommandLink));
    ui->clearHistory->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
    ui->clearSearchBox->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

    ui->downloadsTable->setModel(SettingsPage::downloadsHistoryModel());
    ui->downloadsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->downloadsTable->horizontalHeader()->setSectionResizeMode(SettingsPage::downloadsHistoryModel()->fieldIndex("Page"), QHeaderView::Stretch);
    ui->downloadsTable->setSelectionBehavior(QTableView::SelectRows);
    ui->downloadsTable->verticalHeader()->hide();
    for (int i = 0; i < SettingsPage::downloadsHistoryModel()->columnCount(); i++)
    {
        if (i == SettingsPage::downloadsHistoryModel()->fieldIndex("Id") || i == SettingsPage::downloadsHistoryModel()->fieldIndex("Namespace"))
        {
            ui->downloadsTable->hideColumn(i);
        }
    }
    ui->downloadsTable->setItemDelegateForColumn(SettingsPage::downloadsHistoryModel()->fieldIndex("File"), new DownloadsUrlColumnDelegate());
    ui->downloadsTable->setItemDelegateForColumn(SettingsPage::downloadsHistoryModel()->fieldIndex("Url"), new DownloadsUrlColumnDelegate());
    ui->downloadsTable->setItemDelegateForColumn(SettingsPage::downloadsHistoryModel()->fieldIndex("DateTime"), new DownloadsTimeColumnDelegate());
    ui->downloadsTable->setItemDelegateForColumn(SettingsPage::downloadsHistoryModel()->fieldIndex("Size"), new DownloadsSizeColumnDelegate());

    SettingsPage::downloadsHistoryModel()->select();
    ui->downloadsTable->resizeColumnsToContents();
}

DownloadsPage::~DownloadsPage()
{
    delete ui;
}

void DownloadsPage::on_openFolder_clicked()
{
    QString pathIn = SettingsPage::downloadsHistoryModel()->index(ui->downloadsTable->currentIndex().row(), SettingsPage::downloadsHistoryModel()->fieldIndex("File")).data().toString();

// from Qt Creator source: https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
    {
        // Mac, Windows support folder or file.
    #if defined(Q_OS_WIN)
        QString param;
        if (!QFileInfo(pathIn).isDir())
            param = QLatin1String("/select,");
        param += QDir::toNativeSeparators(pathIn);
        QString command = "explorer.exe " + param;
        QProcess::startDetached(command);
    #elif defined(Q_OS_MAC)
        Q_UNUSED(parent)
        QStringList scriptArgs;
        scriptArgs << QLatin1String("-e")
                   << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                         .arg(pathIn);
        QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
        scriptArgs.clear();
        scriptArgs << QLatin1String("-e")
                   << QLatin1String("tell application \"Finder\" to activate");
        QProcess::execute("/usr/bin/osascript", scriptArgs);
    #else
        // we cannot select a file here, because no file browser really supports it...
        const QFileInfo fileInfo(pathIn);
        const QString folder = fileInfo.absoluteFilePath();
        const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
        QProcess browserProc;
        const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
        if (debug)
            qDebug() <<  browserArgs;
        bool success = browserProc.startDetached(browserArgs);
        const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
        success = success && error.isEmpty();
        if (!success)
            showGraphicalShellError(parent, app, error);
    #endif
    }

}

void DownloadsPage::on_openFile_clicked()
{
    QString pathIn = SettingsPage::downloadsHistoryModel()->index(ui->downloadsTable->currentIndex().row(), SettingsPage::downloadsHistoryModel()->fieldIndex("File")).data().toString();
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(pathIn)));
}

void DownloadsPage::on_openPage_clicked()
{
    QString page = SettingsPage::downloadsHistoryModel()->index(ui->downloadsTable->currentIndex().row(), SettingsPage::downloadsHistoryModel()->fieldIndex("Page")).data().toString();
    emit newWindowLink(QUrl(page));
}

void DownloadsPage::on_clearHistory_clicked()
{
    SettingsPage::clearDownloadsHistory();
}

void DownloadsPage::on_searchBox_textChanged(const QString &arg1)
{
    QStringList cols;
    if (!arg1.isEmpty())
    {
        for (int i = 0; i < SettingsPage::downloadsHistoryModel()->columnCount(); i++)
        {
            if (!ui->downloadsTable->isColumnHidden(i))
            {
                cols << SettingsPage::downloadsHistoryModel()->headerData(i, Qt::Horizontal).toString() +
                        " LIKE \"%" + arg1 + "%\"";
            }
        }
    }
    SettingsPage::downloadsHistoryModel()->setFilter(cols.join(" OR "));
    SettingsPage::downloadsHistoryModel()->select();
}


