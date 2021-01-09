#-------------------------------------------------
#
# Project created by QtCreator 2020-07-11T11:37:33
#
#-------------------------------------------------

# For a release with binary compatibility, cubiomes should be compiled for the
# default achitecture.
QT      += core widgets
LIBS    += -lm $$PWD/cubiomes/libcubiomes.a

win32: {
    LIBS += -static -static-libgcc -static-libstdc++
}

QMAKE_CFLAGS    =  -fwrapv
QMAKE_CXXFLAGS  =  $$QMAKE_CFLAGS
QMAKE_CXXFLAGS_RELEASE *= -O3

TARGET = cubiomes-viewer
#TEMPLATE = app

CONFIG += static

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        aboutdialog.cpp \
        gotodialog.cpp \
        protobasedialog.cpp \
        filterdialog.cpp \
        quadlistdialog.cpp \
        mainwindow.cpp \
        mapview.cpp \
        quad.cpp \
        search.cpp \
        searchthread.cpp \
        main.cpp

HEADERS += \
        cubiomes/finders.h \
        cubiomes/generator.h \
        cubiomes/javarnd.h \
        cubiomes/layers.h \
        cubiomes/util.h \
        aboutdialog.h \
        gotodialog.h \
        protobasedialog.h \
        filterdialog.h \
        quadlistdialog.h \
        mainwindow.h \
        mapview.h \
        quad.h \
        cutil.h \
        search.h \
        searchthread.h

FORMS += \
        aboutdialog.ui \
        gotodialog.ui \
        mainwindow.ui \
        protobasedialog.ui \
        filterdialog.ui \
        quadlistdialog.ui

RESOURCES += \
        icons.qrc

DISTFILES +=
