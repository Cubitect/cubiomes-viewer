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
QMAKE_CXXFLAGS          = $$QMAKE_CFLAGS -std=gnu++11
QMAKE_CXXFLAGS_RELEASE  *= -O3

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
static_gnu: {
    LIBS += -static -static-libgcc -static-libstdc++
}

gcc {
    greaterThan(QMAKE_GCC_MAJOR_VERSION, 9): QMAKE_CXXFLAGS += -Wno-deprecated-copy
}

CONFIG(debug, debug|release): {
    CUTARGET = debug
    !win32 {
        QMAKE_CFLAGS += -fsanitize=undefined
        LIBS += -lubsan -ldl
    }
} else {
    CUTARGET = release
}

# compile cubiomes
CUPATH              = $$PWD/cubiomes
QMAKE_PRE_LINK      += $(MAKE) -C $$CUPATH -f $$CUPATH/makefile CC=\"$$QMAKE_CC\" CFLAGS=\"$$QMAKE_CFLAGS\" $$CUTARGET
QMAKE_CLEAN         += $$CUPATH/*.o $$CUPATH/libcubiomes.a
LIBS                += -lm $$CUPATH/libcubiomes.a

LUAPATH             = $$PWD/lua/src

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
        src/collapsible.cpp \
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
        src/message.cpp \
        src/presetdialog.cpp \
        src/protobasedialog.cpp \
        src/layerdialog.cpp \
        src/mapview.cpp \
        src/rangedialog.cpp \
        src/rangeslider.cpp \
        src/scripts.cpp \
        src/search.cpp \
        src/searchthread.cpp \
        src/tabbiomes.cpp \
        src/tabstructures.cpp \
        src/tabtriggers.cpp \
        src/mainwindow.cpp \
        src/main.cpp \
        src/structuredialog.cpp \
        src/world.cpp

HEADERS += \
        $$CUPATH/finders.h \
        $$CUPATH/generator.h \
        $$CUPATH/layers.h \
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
        src/collapsible.h \
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
        src/message.h \
        src/presetdialog.h \
        src/protobasedialog.h \
        src/layerdialog.h \
        src/mapview.h \
        src/cutil.h \
        src/rangedialog.h \
        src/rangeslider.h \
        src/scripts.h \
        src/search.h \
        src/searchthread.h \
        src/seedtables.h \
        src/tabbiomes.h \
        src/tabstructures.h \
        src/tabtriggers.h \
        src/mainwindow.h \
        src/structuredialog.h \
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
        src/presetdialog.ui \
        src/protobasedialog.ui \
        src/layerdialog.ui \
        src/mainwindow.ui \
        src/rangedialog.ui \
        src/tabbiomes.ui \
        src/tabstructures.ui \
        src/tabtriggers.ui \
        src/structuredialog.ui


RESOURCES += \
        rc/icons.qrc \
        rc/style.qrc \
        rc/examples.qrc \
        rc/lang.qrc


# ----- translations (pluralization) -----

TRANSLATIONS += \
        rc/lang/en_US.ts \
        rc/lang/de_DE.ts \
        rc/lang/zh_CN.ts

translations: {
    # automatically run lupdate for pluralization default translation
    THIS_FILE           = cubiomes-viewer.pro
    lupdate.input       = THIS_FILE
    lupdate.output      = output.dummy.1 # removed by clean
    lupdate.commands    = $$[QT_INSTALL_BINS]/lupdate -pluralonly -noobsolete ${QMAKE_FILE_IN}
    lupdate.CONFIG     += no_link target_predeps

    lrelease.input      = TRANSLATIONS
    lrelease.output     = output.dummy.2 # removed by clean
    lrelease.commands   = $$[QT_INSTALL_BINS]/lrelease ${QMAKE_FILE_IN} \
                            -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_IN_BASE}.qm
    lrelease.CONFIG    += no_link target_predeps

    QMAKE_EXTRA_COMPILERS += lupdate lrelease
    #CONFIG += lrelease embed_translations # does this do anything?
}


# enable network features with: qmake CONFIG+=with_network
with_network: {
    QT += network
    DEFINES += "WITH_UPDATER=1"
    SOURCES += src/updater.cpp
    HEADERS += src/updater.h
}



