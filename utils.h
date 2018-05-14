#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegExp>

namespace Meow
{
    static QString resourceString(QString rc)
    {
        QString str;
        QFile file(rc);
        if (file.open(QFile::ReadOnly))
        {
            str = QTextStream(& file).readAll();
            file.close();
        }
        return str;
    }
    static QRegExp undesiredSymbols(QString str = "\"&,\\:?/*|<>")
    {
        return QRegExp("[" + str + "]");
    }
    static QString excludeUndesiredSymbols(QString source, QString replacer = QString(), QRegExp undesired_symbols = undesiredSymbols())
    {
        return source.replace(undesired_symbols, replacer);
    }
}

#endif // UTILS_H
