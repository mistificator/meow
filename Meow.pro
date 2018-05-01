#-------------------------------------------------
#
# Project created by QtCreator 2018-04-25T20:46:13
#
#-------------------------------------------------

QT       += core gui widgets network svg

LIBS += Qt5WebKit.lib Qt5WebKitWidgets.lib

win32:
{
LIBS += gdiplus.lib
}

TARGET = Meow
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        webpage.cpp \
    filemetadata.cpp \
    settingspage.cpp

HEADERS  += mainwindow.h \
            webpage.h \
    filemetadata.h \
    settingspage.h

FORMS    += mainwindow.ui \
            webpage.ui \
    settingspage.ui
