QT     += core
QT     += gui
QT     += widgets
QT     += concurrent

CONFIG += c++17
CONFIG += lrelease
CONFIG += embed_translations

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

mac {
    lessThan(QT_MAJOR_VERSION, 6) {
        QT += macextras
    }

    CONFIG += app_bundle
    CONFIG += embed_libraries
    CONFIG += embed_translations

    QMAKE_CFLAGS += -mmacosx-version-min=12.2
    QMAKE_CXXFLAGS += -mmacosx-version-min=12.2
    QMAKE_CXXFLAGS += -fno-omit-frame-pointer
    QMAKE_CXXFLAGS += -funwind-tables
    QMAKE_INFO_PLIST = $$PWD/Info.plist

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.5

    # Important for the App with embedded frameworks and libs
    QMAKE_RPATHDIR += @executable_path/../Frameworks
    QMAKE_RPATHDIR += @executable_path/../PlugIns
    QMAKE_RPATHDIR += @executable_path/../lib

    LICENSE.files = $$PWD/LICENSE
    LICENSE.path = Contents/Resources
    QMAKE_BUNDLE_DATA += LICENSE

    privacy.files = $$PWD/privacy.txt
    privacy.path = Contents/Resources
    QMAKE_BUNDLE_DATA += privacy

    icons.files = \
        $$PWD/assets/png/icon_128x128.png \
        $$PWD/assets/png/icon_128x128@2x.png \
        $$PWD/assets/png/icon_16x16.png \
        $$PWD/assets/png/icon_16x16@2x.png \
        $$PWD/assets/png/icon_256x256.png \
        $$PWD/assets/png/icon_256x256@2x.png \
        $$PWD/assets/png/icon_32x32.png \
        $$PWD/assets/png/icon_32x32@2x.png \
        $$PWD/assets/png/icon_512x512.png \
        $$PWD/assets/png/icon_512x512@2x.png \
        $$PWD/assets/png/RoccatTyon.png \
        $$PWD/assets/icns/RoccatTyon_1024x1024.icns \
        $$PWD/assets/icns/RoccatTyon_128x128.icns \
        $$PWD/assets/icns/RoccatTyon_16x16.icns \
        $$PWD/assets/icns/RoccatTyon_256x256.icns \
        $$PWD/assets/icns/RoccatTyon_32x32.icns \
        $$PWD/assets/icns/RoccatTyon_512x512.icns \
        $$PWD/assets/icns/RoccatTyon_64x64.icns
        icons.path = Contents/Resources/
    QMAKE_BUNDLE_DATA += icons
}

SOURCES += \
    main.cpp \
    rtcolordialog.cpp \
    rtdevicecontroller.cpp \
    rthiddevice.cpp \
    rtmainwindow.cpp \
    rtprogress.cpp \
    rtshortcutdialog.cpp

HEADERS += \
    hid_uid.h \
    rtcolordialog.h \
    rtdevicecontroller.h \
    rthiddevice.h \
    rthiddevicedbg.hpp \
    rtmainwindow.h \
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
#HIDAPI_BUILDER = $$PWD/hidapi.sh
#HIDAPI_SOURCE = $$PWD/hidapi
#HIDAPI_TARGET = $$OUT_PWD/.hidapi
#$${HIDAPI_TARGET}.depends = FORCE # or $${PRETARGET}.CONFIG = phony
#$${HIDAPI_TARGET}.commands = $$HIDAPI_BUILDER $$HIDAPI_SOURCE $$OUT_PWD

# build completion tag
#QMAKE_EXTRA_TARGETS += $${HIDAPI_TARGET}
#PRE_TARGETDEPS += $${HIDAPI_TARGET}

#INCLUDEPATH += /usr/local/include
#INCLUDEPATH += /usr/local/include/hidapi
#QMAKE_LIBDIR += /usr/local/lib
#QMAKE_LIBS += -lhidapi
#QMAKE_LIBS += -lusb-1.0

# Default rules for deployment.
target.path = /Application/$${TARGET}
INSTALLS += target

RESOURCES += \
    rtassets.qrc

DISTFILES += \
    LICENSE \
    README.md \
    deploy.sh \
    make.sh
