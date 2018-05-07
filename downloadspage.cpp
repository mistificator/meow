#include "downloadspage.h"
#include "ui_downloadspage.h"
#include "settingspage.h"

#include <QtSql/QSqlQuery>

DownloadsPage::DownloadsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadsPage)
{
    ui->setupUi(this);

    ui->openFolder->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    ui->clearHistory->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
    ui->clearSearchBox->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));

    ui->downloadsTable->setModel(SettingsPage::downloadsHistoryModel());
    ui->downloadsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->downloadsTable->setSelectionBehavior(QTableView::SelectRows);
    ui->downloadsTable->verticalHeader()->hide();
    for (int i = 0; i < SettingsPage::downloadsHistoryModel()->columnCount(); i++)
    {
        if (i == SettingsPage::downloadsHistoryModel()->fieldIndex("Id") || i == SettingsPage::downloadsHistoryModel()->fieldIndex("Namespace"))
        {
            ui->downloadsTable->hideColumn(i);
        }
    }
}

DownloadsPage::~DownloadsPage()
{
    delete ui;
}

void DownloadsPage::on_openFolder_clicked()
{

}

void DownloadsPage::on_clearHistory_clicked()
{
    SettingsPage::clearDownloadsHistory();
}

void DownloadsPage::on_searchBox_textChanged(const QString &arg1)
{

}
