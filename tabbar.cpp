#include "tabbar.h"
#include <QEvent>
#include <QApplication>
#include <QResizeEvent>
#include <QDebug>

TabBar::TabBar(QWidget *_parent)
    : QTabBar(_parent), cached_height(-1), max_tab_size(200)
{
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    setExpanding(false);
    if (topLevelWidget())
    {
        topLevelWidget()->installEventFilter(this);
    }
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

QSize TabBar::minimumSizeHint() const
{
    QSize sz = QTabBar::minimumSizeHint();
    return QSize(0, qMax(sz.height(), 28));
}

QSize TabBar::tabSizeHint(int index) const
{
    const QSize base_hint = QTabBar::tabSizeHint(index);
    if (cached_height == -1)
    {
        cached_height = base_hint.height();
    }

    const int n = qMax(1, count());
    int available_width = parentWidget() ? parentWidget()->width() : width();

    // Резервируем место под фиксированную кнопку справа, чтобы вкладки никогда на нее не наезжали
    if (QTabWidget *tw = qobject_cast<QTabWidget *>(parentWidget()))
    {
        if (QWidget *cw = tw->cornerWidget(Qt::TopRightCorner))
        {
            const int cw_width = qMax(cw->width(), cw->sizeHint().width());
            available_width -= (cw_width + 8);
        }
    }

    const int tab_width = qBound(60, available_width / n, max_tab_size);
    return QSize(tab_width, cached_height);
}
