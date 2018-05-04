#include "tabbar.h"
#include <QEvent>
#include <QApplication>
#include <QResizeEvent>
#include <QDebug>

TabBar::TabBar(QWidget * _parent): QTabBar(_parent), cached_height(-1), max_tab_size(200)
{
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    topLevelWidget()->installEventFilter(this);
}

void TabBar::setMaximumTabSize(int sz)
{
    max_tab_size = sz;
}

int TabBar::maximumTabSize() const
{
    return max_tab_size;
}

void TabBar::tabLayoutChange()
{
    updateGeometry();
}

bool TabBar::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize || event->type() == QEvent::WindowStateChange)
    {
        cached_height = -1;
        qApp->postEvent(this, new QResizeEvent(QSize(), QSize()));
    }
    return QTabBar::eventFilter(watched, event);
}

// autosize tabs from http://www.prog.org.ru/topic_24142_0.html
QSize TabBar::tabSizeHint(int index) const
{
    const QSize last_section_hint = QTabBar::tabSizeHint(count() - 1);
    if (index == count() - 1)
    {
        return last_section_hint;
    }
    if (cached_height == -1)
    {
        cached_height = QTabBar::tabSizeHint(index).height();
    }
    return QSize(qBound(1, (parentWidget()->width() - last_section_hint.width()) / (count() - 1), max_tab_size), cached_height);
}
