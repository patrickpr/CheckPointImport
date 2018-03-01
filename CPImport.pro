#-------------------------------------------------
#
# Project created by QtCreator 2017-10-29T09:51:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CPImport
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    src/app/main.cpp \
    src/app/mainimportwin.cpp \
    src/app/dialoglogin.cpp \
    src/app/cmgmtconnect.cpp \
    src/app/cobjectdlg.cpp \
    src/app/cversioncheck.cpp

HEADERS += \
    src/app/mainimportwin.h \
    src/app/dialoglogin.h \
    src/app/cmgmtconnect.h \
    src/app/cobjectdlg.h \
    src/app/cversioncheck.h

FORMS += \
    src/app/mainimportwin.ui \
    src/app/dialoglogin.ui \
    src/app/cobjectdlg.ui

INCLUDEPATH += "src/openssl/include"

LIBS += -L"$$PWD/src/openssl/lib/MinGW"
LIBS += "-llibeay32"
LIBS += "-lssleay32"

DISTFILES +=

RESOURCES += \
    resource.qrc
