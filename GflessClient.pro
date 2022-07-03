QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

RC_ICONS = resources/gfless_icon.ico

QMAKE_LFLAGS_WINDOWS += "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""

LIBS += -lkernel32 -luser32 -lgdi32

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    account.cpp \
    addaccountdialog.cpp \
    addprofileaccountDialog.cpp \
    addprofiledialog.cpp \
    blackbox.cpp \
    fingerprint.cpp \
    gflessclient.cpp \
    identity.cpp \
    nostaleauth.cpp \
    profile.cpp \
    settingsdialog.cpp \
    syncnetworkaccessmanager.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    account.h \
    addaccountdialog.h \
    addprofileaccountdialog.h \
    addprofiledialog.h \
    blackbox.h \
    fingerprint.h \
    gflessclient.h \
    identity.h \
    injector.h \
    mainwindow.h \
    nostaleauth.h \
    processchecker.h \
    profile.h \
    settingsdialog.h \
    syncnetworkaccessmanager.h

FORMS += \
    addaccountdialog.ui \
    addprofileaccountdialog.ui \
    addprofiledialog.ui \
    mainwindow.ui \
    settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


RESOURCES += \
    resources.qrc
