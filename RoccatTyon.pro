QT     += core
QT     += gui
QT     += widgets
QT     += concurrent

CONFIG += c++17
CONFIG += lrelease
CONFIG += embed_translations

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

mac {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.2
    QMAKE_CFLAGS += -mmacosx-version-min=12.2
    QMAKE_CXXFLAGS += -mmacosx-version-min=12.2
    QMAKE_CXXFLAGS += -fno-omit-frame-pointer
    QMAKE_CXXFLAGS += -funwind-tables
}

SOURCES += \
    main.cpp \
    rtcolordialog.cpp \
    rtdevicecontroller.cpp \
    rthiddevice.cpp \
    rtmainwindow.cpp \
    rtprofilemodel.cpp \
    rtprogress.cpp \
    rtshortcutdialog.cpp

HEADERS += \
    hid_uid.h \
    rtcolordialog.h \
    rtdevicecontroller.h \
    rthiddevice.h \
    rthiddevicedbg.hpp \
    rtmainwindow.h \
    rtprofilemodel.h \
    rtprogress.h \
    rtshortcutdialog.h \
    rttypes.h

FORMS += \
    rtcolordialog.ui \
    rtmainwindow.ui \
    rtshortcutdialog.ui

TRANSLATIONS += \
    RoccatTyon_en_US.ts

### HIDAPI/LIBUSB
HIDAPI_BUILDER = $$PWD/hidapi.sh
HIDAPI_SOURCE = $$PWD/hidapi
HIDAPI_TARGET = $$OUT_PWD/.hidapi
$${HIDAPI_TARGET}.depends = FORCE # or $${PRETARGET}.CONFIG = phony
$${HIDAPI_TARGET}.commands = $$HIDAPI_BUILDER $$HIDAPI_SOURCE $$OUT_PWD

# build completion tag
QMAKE_EXTRA_TARGETS += $${HIDAPI_TARGET}
PRE_TARGETDEPS += $${HIDAPI_TARGET}

INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/hidapi
QMAKE_LIBDIR += /usr/local/lib
QMAKE_LIBS += -lhidapi
#QMAKE_LIBS += -lusb-1.0

# Default rules for deployment.
target.path = /Application/$${TARGET}
INSTALLS += target

RESOURCES += \
    rtassets.qrc

DISTFILES += \
    .gitignore \
    .gitmodules \
    hidapi.sh \
    linux-app-reset-3t-sniff.txt \
    linux-app-reset-3u-sniff.txt \
    linux-app-saveall-lightchg-sniff.txt \
    linux-app-saveall-sniff.txt \
    linux-app-start-sniff.txt \
    roccat.md \
    tyon_hid_todo.txt \
    writeByHidApi.txt
