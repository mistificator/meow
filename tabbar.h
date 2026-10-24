#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include <QTabWidget>

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit TabBar(QWidget *_parent = nullptr);
    void setMaximumTabSize(int);
    int maximumTabSize() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    QSize tabSizeHint(int index) const override;
    QSize minimumSizeHint() const override;
    void tabLayoutChange() override;

private:
    mutable int cached_height;
    int max_tab_size;
};

class TabWidgetOpener : public QTabWidget
{
    Q_OBJECT
public:
    static QTabBar *tabBar(QTabWidget *tw)
    {
        return static_cast<TabWidgetOpener *>(tw)->QTabWidget::tabBar();
    }
    static void setTabBar(QTabWidget *tw, QTabBar *tb)
    {
        static_cast<TabWidgetOpener *>(tw)->QTabWidget::setTabBar(tb);
    }
};

#endif // TABBAR_H
