#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include <QTabWidget>

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    TabBar(QWidget * _parent = 0);
    void setMaximumTabSize(int);
    int maximumTabSize() const;
protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);
    virtual QSize tabSizeHint(int index) const;
    virtual void tabLayoutChange();
private:
    mutable int cached_height;
    int max_tab_size;
};

class TabWidgetOpener: public QTabWidget
{
    Q_OBJECT
public:
    static QTabBar * tabBar(QTabWidget * tw)
    {
        return ((TabWidgetOpener *)tw)->QTabWidget::tabBar();
    }
    static void setTabBar(QTabWidget * tw, QTabBar * tb)
    {
        ((TabWidgetOpener *)tw)->QTabWidget::setTabBar(tb);
    }
};

#endif // TABBAR_H
