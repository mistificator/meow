#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QApplication>

namespace Meow
{
static QString resourceString(const QString &rc)
{
    QFile file(rc);
    if (file.open(QFile::ReadOnly))
    {
        return QTextStream(&file).readAll();
    }
    return QString();
}

static QRegularExpression undesiredSymbols(const QString &str = "\"&,\\:?/*|<>")
{
    return QRegularExpression("[" + QRegularExpression::escape(str) + "]");
}

static QString excludeUndesiredSymbols(QString source,
                                       const QString &replacer = QString(),
                                       const QRegularExpression &undesired_symbols = undesiredSymbols())
{
    return source.replace(undesired_symbols, replacer);
}

static void waitEventsProcessing()
{
    qApp->sendPostedEvents();
    qApp->processEvents();
}
}

#endif // UTILS_H
