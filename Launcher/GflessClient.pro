QT       += core gui network webenginecore

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

RC_ICONS = resources/gfless_icon.ico

QMAKE_LFLAGS_WINDOWS += "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""

LIBS += -lkernel32 -luser32 -lgdi32

VERSION = 1.0.11

INCLUDEPATH += ./src ./src/gui ./src/auth


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/auth/blackboxgenerator.cpp \
    src/auth/gameupdater.cpp \
    src/gameaccount.cpp \
    src/gameforgeaccount.cpp \
    src/gui/addaccountdialog.cpp \
    src/gui/addprofileaccountdialog.cpp \
    src/gui/addprofiledialog.cpp \
    src/auth/blackbox.cpp \
    src/gui/captchadialog.cpp \
    src/auth/captchasolver.cpp \
    src/auth/fingerprint.cpp \
    src/auth/gflessclient.cpp \
    src/auth/identity.cpp \
    src/gui/creategameaccountdialog.cpp \
    src/gui/editmultipleprofileaccountsdialog.cpp \
    src/gui/gameupdatedialog.cpp \
    src/gui/identitydialog.cpp \
    src/auth/nostaleauth.cpp \
    src/profile.cpp \
    src/gui/settingsdialog.cpp \
    src/syncnetworkaccessmanager.cpp \
    src/main.cpp \
    src/gui/mainwindow.cpp

HEADERS += \
    src/auth/blackboxgenerator.h \
    src/auth/gameupdater.h \
    src/gameaccount.h \
    src/gameforgeaccount.h \
    src/gui/addaccountdialog.h \
    src/gui/addprofileaccountdialog.h \
    src/gui/addprofiledialog.h \
    src/auth/blackbox.h \
    src/gui/captchadialog.h \
    src/auth/captchasolver.h \
    src/auth/fingerprint.h \
    src/auth/gflessclient.h \
    src/auth/identity.h \
    src/gui/creategameaccountdialog.h \
    src/gui/editmultipleprofileaccountsdialog.h \
    src/gui/gameupdatedialog.h \
    src/gui/identitydialog.h \
    src/gui/mainwindow.h \
    src/auth/nostaleauth.h \
    src/processchecker.h \
    src/profile.h \
    src/gui/settingsdialog.h \
    src/syncnetworkaccessmanager.h

FORMS += \
    src/gui/addaccountdialog.ui \
    src/gui/addprofileaccountdialog.ui \
    src/gui/addprofiledialog.ui \
    src/gui/captchadialog.ui \
    src/gui/creategameaccountdialog.ui \
    src/gui/editmultipleprofileaccountsdialog.ui \
    src/gui/gameupdatedialog.ui \
    src/gui/identitydialog.ui \
    src/gui/mainwindow.ui \
    src/gui/settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


RESOURCES += \
    resources.qrc

