#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include <QTabWidget>

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    TabBar(QWidget * _parent = 0);
protected:
    bool eventFilter(QObject *watched, QEvent *event);
    QSize tabSizeHint(int index) const;
private:
    mutable int cachedHeight;
private slots:
    void rescale();
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
