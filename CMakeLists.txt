cmake_minimum_required(VERSION 3.16)
project(Meow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Svg
    Sql
    Qml
    WebEngineCore
    WebEngineWidgets
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    webpage.cpp
    webpage.h
    webpage.ui
    filemetadata.cpp
    filemetadata.h
    settingspage.cpp
    settingspage.h
    settingspage.ui
    tabbar.cpp
    tabbar.h
    downloadspage.cpp
    downloadspage.h
    downloadspage.ui
    downloadmanager.cpp
    downloadmanager.h
    filemanager.cpp
    filemanager.h
    version.h
    utils.h
    custom_classes.h
    web_mime_types.h
    meow.qrc
)

if(WIN32)
    list(APPEND PROJECT_SOURCES Meow.rc)
endif()

qt_add_executable(Meow WIN32 ${PROJECT_SOURCES})

target_link_libraries(Meow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::Sql
    Qt6::Qml
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
)

if(WIN32)
    target_link_libraries(Meow PRIVATE gdiplus)
endif()
