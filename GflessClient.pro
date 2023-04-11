QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

RC_ICONS = resources/gfless_icon.ico

QMAKE_LFLAGS_WINDOWS += "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""

LIBS += -lkernel32 -luser32 -lgdi32

INCLUDEPATH += ./src

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/account.cpp \
    src/addaccountdialog.cpp \
    src/addprofileaccountDialog.cpp \
    src/addprofiledialog.cpp \
    src/blackbox.cpp \
    src/captchadialog.cpp \
    src/captchasolver.cpp \
    src/fingerprint.cpp \
    src/gflessclient.cpp \
    src/identity.cpp \
    src/identitydialog.cpp \
    src/nostaleauth.cpp \
    src/profile.cpp \
    src/settingsdialog.cpp \
    src/syncnetworkaccessmanager.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/account.h \
    src/addaccountdialog.h \
    src/addprofileaccountdialog.h \
    src/addprofiledialog.h \
    src/blackbox.h \
    src/captchadialog.h \
    src/captchasolver.h \
    src/fingerprint.h \
    src/gflessclient.h \
    src/identity.h \
    src/identitydialog.h \
    src/injector.h \
    src/mainwindow.h \
    src/nostaleauth.h \
    src/processchecker.h \
    src/profile.h \
    src/settingsdialog.h \
    src/syncnetworkaccessmanager.h

FORMS += \
    src/addaccountdialog.ui \
    src/addprofileaccountdialog.ui \
    src/addprofiledialog.ui \
    src/captchadialog.ui \
    src/identitydialog.ui \
    src/mainwindow.ui \
    src/settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


RESOURCES += \
    resources.qrc

