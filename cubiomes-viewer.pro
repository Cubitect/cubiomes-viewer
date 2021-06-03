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
QMAKE_CXXFLAGS  =  $$QMAKE_CFLAGS -std=gnu++11 -Wno-deprecated-copy -Wno-missing-field-initilizers
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
        src/aboutdialog.cpp \
        src/collapsable.cpp \
        src/configdialog.cpp \
        src/formconditions.cpp \
        src/formgen48.cpp \
        src/formsearchcontrol.cpp \
        src/gotodialog.cpp \
        src/protobasedialog.cpp \
        src/filterdialog.cpp \
        src/quadlistdialog.cpp \
        src/mapview.cpp \
        src/quad.cpp \
        src/search.cpp \
        src/searchitem.cpp \
        src/searchthread.cpp \
        src/mainwindow.cpp \
        src/main.cpp

HEADERS += \
        cubiomes/finders.h \
        cubiomes/generator.h \
        cubiomes/javarnd.h \
        cubiomes/layers.h \
        cubiomes/util.h \
        src/collapsable.h \
        src/aboutdialog.h \
        src/configdialog.h \
        src/formconditions.h \
        src/formgen48.h \
        src/formsearchcontrol.h \
        src/gotodialog.h \
        src/protobasedialog.h \
        src/filterdialog.h \
        src/quadlistdialog.h \
        src/mapview.h \
        src/quad.h \
        src/cutil.h \
        src/search.h \
        src/searchitem.h \
        src/searchthread.h \
        src/seedtables.h \
        src/mainwindow.h \
        src/settings.h

FORMS += \
        src/aboutdialog.ui \
        src/configdialog.ui \
        src/formconditions.ui \
        src/formgen48.ui \
        src/formsearchcontrol.ui \
        src/gotodialog.ui \
        src/protobasedialog.ui \
        src/filterdialog.ui \
        src/quadlistdialog.ui\
        src/mainwindow.ui

RESOURCES += \
        icons.qrc

DISTFILES +=
