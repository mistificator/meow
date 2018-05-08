#-------------------------------------------------
#
# Project created by QtCreator 2018-04-25T20:46:13
#
#-------------------------------------------------

# don't forget to exclude debug_and_release, cause qmake is insane
# https://stackoverflow.com/questions/19562591/qt-creator-config-debug-release-switches-does-not-work
CONFIG -= debug_and_release

QT       += core gui widgets network svg sql

unix: QT += webkit webkitwidgets
win32:
{
    QT_LIB_PATH = $$[QT_INSTALL_LIBS]
    CONFIG(debug, debug|release) {
        LIBS += $$QT_LIB_PATH/Qt5WebKitd.lib $$QT_LIB_PATH/Qt5WebKitWidgetsd.lib
    }
    CONFIG(release, debug|release) {
        LIBS += $$QT_LIB_PATH/Qt5WebKit.lib $$QT_LIB_PATH/Qt5WebKitWidgets.lib
    }
    LIBS += gdiplus.lib
}

TARGET =    Meow
TEMPLATE =  app


SOURCES += main.cpp\
        mainwindow.cpp \
        webpage.cpp \
        filemetadata.cpp \
        settingspage.cpp \
        tabbar.cpp \
    downloadspage.cpp

HEADERS  += mainwindow.h \
            webpage.h \
            filemetadata.h \
            settingspage.h \
            tabbar.h \
            version.h \
    downloadspage.h \
    utils.h

FORMS    += mainwindow.ui \
            webpage.ui \
            settingspage.ui \
    downloadspage.ui

win32:
{
    DISTFILES += Meow.rc
    RC_FILE += Meow.rc
}

RESOURCES += \
    meow.qrc
