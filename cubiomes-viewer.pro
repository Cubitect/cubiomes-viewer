#-------------------------------------------------
#
# Project created by QtCreator 2020-07-11T11:37:33
#
#-------------------------------------------------

QT += core widgets

# uncomment to override the profile compiler
#QMAKE_CC = clang
#QMAKE_CXX = clang++

CHARSET                 = -finput-charset=UTF-8 -fexec-charset=UTF-8
QMAKE_CFLAGS            = $$CHARSET -fwrapv -DSTRUCT_CONFIG_OVERRIDE=1
QMAKE_CXXFLAGS          = $$QMAKE_CFLAGS
QMAKE_CXXFLAGS_RELEASE  *= -O3 -g3

greaterThan(QT_MAJOR_VERSION, 5) {
    QMAKE_CXXFLAGS += -std=gnu++17
    DEFINES += QT_DISABLE_DEPRECATED_UP_TO=0x050F00
} else {
    QMAKE_CXXFLAGS += -std=gnu++11
}

win32: {
    CONFIG += static_gnu

    # thank you nullprogram for dealing with the Windows UTF-16 nonsense
    LIBWINSANE          = $$PWD/src/libwinsane
    libwinsane.target   = libwinsane
    libwinsane.output   = $$LIBWINSANE/libwinsane.o
    libwinsane.commands = $(MAKE) -C $$LIBWINSANE -f $$LIBWINSANE/Makefile
    QMAKE_EXTRA_TARGETS += libwinsane
    PRE_TARGETDEPS      += libwinsane
    LIBS                += $$LIBWINSANE/libwinsane.o
} else {
    DEFINES += "LUA_USE_POSIX=1"
}

wasm: {
    DEFINES += "WASM=1"
    #QT_WASM_SOURCE_MAP=1
    QT_WASM_INITIAL_MEMORY = 256MB
    QT_WASM_PTHREAD_POOL_SIZE = 32
    CONFIG(debug, debug|release): {
        #QMAKE_CFLAGS += -O3 -gsource-map
    }
}
#CONFIG += sanitizer
#CONFIG += sanitize_undefined
#CONFIG += sanitize_thread

static_gnu: {
    LIBS += -static -static-libgcc -static-libstdc++
}

gcc {
    greaterThan(QMAKE_GCC_MAJOR_VERSION, 9): QMAKE_CXXFLAGS += -Wno-deprecated-copy
}

CONFIG(debug, debug|release): {
    CUTARGET = debug
} else {
    CUTARGET = release
}

# compile cubiomes
CUPATH              = $$PWD/cubiomes
QMAKE_PRE_LINK      += $(MAKE) -C $$CUPATH -f $$CUPATH/makefile CC=\"$$QMAKE_CC\" CFLAGS=\"$(CFLAGS) $$QMAKE_CFLAGS\" $$CUTARGET
QMAKE_CLEAN         += $$CUPATH/*.o $$CUPATH/libcubiomes.a
LIBS                += $$CUPATH/libcubiomes.a -lm

LUAPATH = $$PWD/lua/src

TARGET = cubiomes-viewer

SOURCES += \
        $$LUAPATH/lapi.c \
        $$LUAPATH/lauxlib.c \
        $$LUAPATH/lbaselib.c \
        $$LUAPATH/lcode.c \
        $$LUAPATH/lcorolib.c \
        $$LUAPATH/lctype.c \
        $$LUAPATH/ldblib.c \
        $$LUAPATH/ldebug.c \
        $$LUAPATH/ldo.c \
        $$LUAPATH/ldump.c \
        $$LUAPATH/lfunc.c \
        $$LUAPATH/lgc.c \
        $$LUAPATH/linit.c \
        $$LUAPATH/liolib.c \
        $$LUAPATH/llex.c \
        $$LUAPATH/lmathlib.c \
        $$LUAPATH/lmem.c \
        $$LUAPATH/loadlib.c \
        $$LUAPATH/lobject.c \
        $$LUAPATH/lopcodes.c \
        $$LUAPATH/loslib.c \
        $$LUAPATH/lparser.c \
        $$LUAPATH/lstate.c \
        $$LUAPATH/lstring.c \
        $$LUAPATH/lstrlib.c \
        $$LUAPATH/ltable.c \
        $$LUAPATH/ltablib.c \
        $$LUAPATH/ltm.c \
        $$LUAPATH/lundump.c \
        $$LUAPATH/lutf8lib.c \
        $$LUAPATH/lvm.c \
        $$LUAPATH/lzio.c \
        src/aboutdialog.cpp \
        src/biomecolordialog.cpp \
        src/conditiondialog.cpp \
        src/config.cpp \
        src/configdialog.cpp \
        src/extgendialog.cpp \
        src/exportdialog.cpp \
        src/formconditions.cpp \
        src/formgen48.cpp \
        src/formsearchcontrol.cpp \
        src/gotodialog.cpp \
        src/headless.cpp \
        src/maptoolsdialog.cpp \
        src/message.cpp \
        src/presetdialog.cpp \
        src/layerdialog.cpp \
        src/mapview.cpp \
        src/rangedialog.cpp \
        src/scripts.cpp \
        src/search.cpp \
        src/searchthread.cpp \
        src/tabbiomes.cpp \
        src/tablocations.cpp \
        src/tabstructures.cpp \
        src/mainwindow.cpp \
        src/main.cpp \
        src/util.cpp \
        src/widgets.cpp \
        src/world.cpp

HEADERS += \
        $$CUPATH/finders.h \
        $$CUPATH/generator.h \
        $$CUPATH/layers.h \
        $$CUPATH/biomes.h \
        $$CUPATH/quadbase.h \
        $$CUPATH/util.h \
        $$LUAPATH/lapi.h \
        $$LUAPATH/lauxlib.h \
        $$LUAPATH/lcode.h \
        $$LUAPATH/lctype.h \
        $$LUAPATH/ldebug.h \
        $$LUAPATH/ldo.h \
        $$LUAPATH/lfunc.h \
        $$LUAPATH/lgc.h \
        $$LUAPATH/ljumptab.h \
        $$LUAPATH/llex.h \
        $$LUAPATH/llimits.h \
        $$LUAPATH/lmem.h \
        $$LUAPATH/lobject.h \
        $$LUAPATH/lopcodes.h \
        $$LUAPATH/lopnames.h \
        $$LUAPATH/lparser.h \
        $$LUAPATH/lprefix.h \
        $$LUAPATH/lstate.h \
        $$LUAPATH/lstring.h \
        $$LUAPATH/ltable.h \
        $$LUAPATH/ltm.h \
        $$LUAPATH/lua.h \
        $$LUAPATH/lua.hpp \
        $$LUAPATH/luaconf.h \
        $$LUAPATH/lualib.h \
        $$LUAPATH/lundump.h \
        $$LUAPATH/lvm.h \
        $$LUAPATH/lzio.h \
        src/aboutdialog.h \
        src/biomecolordialog.h \
        src/conditiondialog.h \
        src/config.h \
        src/configdialog.h \
        src/extgendialog.h \
        src/exportdialog.h \
        src/formconditions.h \
        src/formgen48.h \
        src/formsearchcontrol.h \
        src/gotodialog.h \
        src/headless.h \
        src/maptoolsdialog.h \
        src/message.h \
        src/presetdialog.h \
        src/layerdialog.h \
        src/mapview.h \
        src/qzipwriter.h \
        src/rangedialog.h \
        src/scripts.h \
        src/search.h \
        src/searchthread.h \
        src/seedtables.h \
        src/tabbiomes.h \
        src/tablocations.h \
        src/tabstructures.h \
        src/mainwindow.h \
        src/util.h \
        src/widgets.h \
        src/world.h

FORMS += \
        src/aboutdialog.ui \
        src/biomecolordialog.ui \
        src/conditiondialog.ui \
        src/configdialog.ui \
        src/extgendialog.ui \
        src/exportdialog.ui \
        src/formconditions.ui \
        src/formgen48.ui \
        src/formsearchcontrol.ui \
        src/gotodialog.ui \
        src/maptoolsdialog.ui \
        src/presetdialog.ui \
        src/layerdialog.ui \
        src/mainwindow.ui \
        src/rangedialog.ui \
        src/tabbiomes.ui \
        src/tablocations.ui \
        src/tabstructures.ui

RESOURCES += \
        rc/icons.qrc \
        rc/style.qrc \
        rc/examples.qrc \
        rc/lang.qrc \
        rc/qh.qrc

# ----- translations (pluralization) -----

TRANSLATIONS += \
        rc/lang/en_US.ts \
        rc/lang/de_DE.ts \
        rc/lang/zh_CN.ts


# enable network features with: qmake CONFIG+=with_network
with_network: {
    QT += network
    DEFINES += "WITH_UPDATER=1"
    SOURCES += src/updater.cpp
    HEADERS += src/updater.h
}

# enable dbus features with: qmake CONFIG+=with_dbus
with_dbus: {
    QT += dbus
    DEFINES += "WITH_DBUS=1"
}
