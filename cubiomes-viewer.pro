#-------------------------------------------------
#
# Project created by QtCreator 2020-07-11T11:37:33
#
#-------------------------------------------------

CUPATH   = $$PWD/cubiomes
QT      += core widgets
LIBS    += -lm $$CUPATH/libcubiomes.a

# uncomment to override the profile compiler
#QMAKE_CC = clang
#QMAKE_CXX = clang++

QMAKE_CFLAGS    = -fwrapv -DSTRUCT_CONFIG_OVERRIDE=1
QMAKE_CXXFLAGS  = $$QMAKE_CFLAGS -std=gnu++11
QMAKE_CXXFLAGS_RELEASE *= -O3

win32: {
    LIBS += -static -static-libgcc -static-libstdc++
}

# compile cubiomes
release: {
    CUTARGET = release
} else: { # may need the release target to be disabled: qmake CONFIG-=release
    CUTARGET = debug
}
QMAKE_PRE_LINK += $(MAKE) -C $$CUPATH -f $$CUPATH/makefile CFLAGS="-DSTRUCT_CONFIG_OVERRIDE=1" $$CUTARGET
QMAKE_CLEAN += $$CUPATH/*.o $$CUPATH/libcubiomes.a

TARGET = cubiomes-viewer

CONFIG += static


SOURCES += \
        src/aboutdialog.cpp \
        src/collapsible.cpp \
        src/configdialog.cpp \
        src/examplesdialog.cpp \
        src/extgendialog.cpp \
        src/formconditions.cpp \
        src/formgen48.cpp \
        src/formsearchcontrol.cpp \
        src/gotodialog.cpp \
        src/protobasedialog.cpp \
        src/filterdialog.cpp \
        src/quadlistdialog.cpp \
        src/mapview.cpp \
        src/quad.cpp \
        src/rangedialog.cpp \
        src/rangeslider.cpp \
        src/search.cpp \
        src/searchitem.cpp \
        src/searchthread.cpp \
        src/mainwindow.cpp \
        src/main.cpp

HEADERS += \
        $$CUPATH/finders.h \
        $$CUPATH/generator.h \
        $$CUPATH/layers.h \
        $$CUPATH/util.h \
        src/aboutdialog.h \
        src/collapsible.h \
        src/configdialog.h \
        src/examplesdialog.h \
        src/extgendialog.h \
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
        src/rangedialog.h \
        src/rangeslider.h \
        src/search.h \
        src/searchitem.h \
        src/searchthread.h \
        src/seedtables.h \
        src/mainwindow.h \
        src/settings.h

FORMS += \
        src/aboutdialog.ui \
        src/configdialog.ui \
        src/examplesdialog.ui \
        src/extgendialog.ui \
        src/formconditions.ui \
        src/formgen48.ui \
        src/formsearchcontrol.ui \
        src/gotodialog.ui \
        src/protobasedialog.ui \
        src/filterdialog.ui \
        src/quadlistdialog.ui\
        src/mainwindow.ui \
        src/rangedialog.ui

RESOURCES += \
        rc/icons.qrc \
        rc/style.qrc \
        rc/examples.qrc


# disable network features completely with: qmake CONFIG+=without_network
!without_network: {
    QT += network
    DEFINES += "WITH_UPDATER=1"
    SOURCES += src/updater.cpp
    HEADERS += src/updater.h
}



