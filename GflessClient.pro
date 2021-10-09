QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

QMAKE_LFLAGS_WINDOWS += "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""

LIBS += -lkernel32

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AddAccountDialog.cpp \
    GflessClient.cpp \
    NostaleAuth.cpp \
    QSyncNetworkManager.cpp \
    SettingsDialog.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    AddAccountDialog.h \
    GflessClient.h \
    MainWindow.h \
    NostaleAuth.h \
    ProcessChecker.h \
    QSyncNetworkManager.h \
    SettingsDialog.h

FORMS += \
    AddAccountDialog.ui \
    MainWindow.ui \
    SettingsDialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

unix|win32: LIBS += -L$$PWD/../../../../DEV/vcpkg/installed/x86-windows/lib/ -llibcurl

INCLUDEPATH += $$PWD/../../../../DEV/vcpkg/installed/x86-windows/include
DEPENDPATH += $$PWD/../../../../DEV/vcpkg/installed/x86-windows/include

RESOURCES += \
    resources.qrc
