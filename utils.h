#ifndef UTILS_H
#define UTILS_H

#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace Meow
{
    QString resourceString(QString rc)
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
}

#endif // UTILS_H
