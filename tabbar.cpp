#include "tabbar.h"
#include <QEvent>
#include <QApplication>
#include <QResizeEvent>
#include <QDebug>

TabBar::TabBar(QWidget * _parent): QTabBar(_parent), cachedHeight(-1)
{
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    topLevelWidget()->installEventFilter(this);
}

void TabBar::rescale()
{
    updateGeometry();
}

bool TabBar::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize || event->type() == QEvent::WindowStateChange)
    {
        cachedHeight = -1;
        qApp->postEvent(this, new QResizeEvent(QSize(), QSize()));
        QMetaObject::invokeMethod(this, "rescale", Qt::QueuedConnection);
    }
    return QTabBar::eventFilter(watched, event);
}

// autosize tabs from http://www.prog.org.ru/topic_24142_0.html

QSize TabBar::tabSizeHint(int index) const
{
    if (index == count() - 1)
    {
        return QTabBar::tabSizeHint(index);
    }
    if (cachedHeight == -1)
    {
        cachedHeight = QTabBar::tabSizeHint(index).height();
    }
    return QSize(qBound(1, parentWidget()->width()/count(), 200), cachedHeight);
}
