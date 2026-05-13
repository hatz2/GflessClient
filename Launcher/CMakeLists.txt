cmake_minimum_required(VERSION 3.21)
project(GflessClientNoIP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebEngineCore WebChannel)

qt_add_executable(GflessClientNoIP WIN32
    src/auth/blackboxgenerator.cpp
    src/auth/gameupdater.cpp
    src/gameaccount.cpp
    src/gameforgeaccount.cpp
    src/gui/addaccountdialog.cpp
    src/gui/addprofileaccountdialog.cpp
    src/gui/addprofiledialog.cpp
    src/auth/blackbox.cpp
    src/gui/captchadialog.cpp
    src/auth/captchasolver.cpp
    src/auth/fingerprint.cpp
    src/auth/gflessclient.cpp
    src/auth/identity.cpp
    src/gui/creategameaccountdialog.cpp
    src/gui/editmultipleprofileaccountsdialog.cpp
    src/gui/gameupdatedialog.cpp
    src/gui/identitydialog.cpp
    src/auth/nostaleauth.cpp
    src/profile.cpp
    src/gui/settingsdialog.cpp
    src/syncnetworkaccessmanager.cpp
    src/main.cpp
    src/gui/mainwindow.cpp

    src/auth/blackboxgenerator.h
    src/auth/gameupdater.h
    src/gameaccount.h
    src/gameforgeaccount.h
    src/gui/addaccountdialog.h
    src/gui/addprofileaccountdialog.h
    src/gui/addprofiledialog.h
    src/auth/blackbox.h
    src/gui/captchadialog.h
    src/auth/captchasolver.h
    src/auth/fingerprint.h
    src/auth/gflessclient.h
    src/auth/identity.h
    src/gui/creategameaccountdialog.h
    src/gui/editmultipleprofileaccountsdialog.h
    src/gui/gameupdatedialog.h
    src/gui/identitydialog.h
    src/gui/mainwindow.h
    src/auth/nostaleauth.h
    src/processchecker.h
    src/profile.h
    src/gui/settingsdialog.h
    src/syncnetworkaccessmanager.h

    src/gui/addaccountdialog.ui
    src/gui/addprofileaccountdialog.ui
    src/gui/addprofiledialog.ui
    src/gui/captchadialog.ui
    src/gui/creategameaccountdialog.ui
    src/gui/editmultipleprofileaccountsdialog.ui
    src/gui/gameupdatedialog.ui
    src/gui/identitydialog.ui
    src/gui/mainwindow.ui
    src/gui/settingsdialog.ui

    resources.qrc
)

target_include_directories(GflessClientNoIP PRIVATE
    src
    src/gui
    src/auth
)

target_compile_definitions(GflessClientNoIP PRIVATE NO_PROXY_MODE)

target_link_libraries(GflessClientNoIP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineCore
    Qt6::WebChannel
)
