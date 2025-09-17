# List all modules that Xcode need for embedding
QT  += core
QT  += gui
QT  += widgets
QT  += concurrent
QT  += core5compat
QT  += network
QT  += dbus
QT  += opengl
QT  += svg
QT  += quick
QT  += quick3d
QT  += quick3dutils
QT  += quick3druntimerender
#QT  += quickshapes
QT  += qml
QT  += qmlmeta
QT  += qmlmodels
QT  += qmlworkerscript

CONFIG += c++17
CONFIG += lrelease
CONFIG += embed_translations

# define where frameworks and plugins exist
HOME = $$system(echo $HOME)
QTDIR = $$HOME/Qt/6.9.1/macos

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

    QMAKE_CFLAGS += -mmacosx-version-min=13.5
    QMAKE_CXXFLAGS += -mmacosx-version-min=13.5
    QMAKE_CXXFLAGS += -fno-omit-frame-pointer
    QMAKE_CXXFLAGS += -funwind-tables

    QMAKE_INFO_PLIST = $$PWD/Info.plist

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.5

    # Objective-C compiler
    OBJECTIVE_SOURCES += $$PWD/rtmacoshelper.mm
    OBJECTIVE_HEADERS += $$PWD/rtmacoshelper.h

    # Important for the App with embedded frameworks and libs
    #QMAKE_RPATHDIR += @executable_path/../Frameworks
    QMAKE_RPATHDIR += @executable_path/../PlugIns
    QMAKE_RPATHDIR += @executable_path/../lib

    LIBS += -framework QtCore
    LIBS += -framework QtConcurrent
    LIBS += -framework QtCore5Compat
    LIBS += -framework QtNetwork
    LIBS += -framework QtDBus
    LIBS += -framework QtShaderTools
    LIBS += -framework QtOpenGL
    LIBS += -framework QtGraphs
    LIBS += -framework QtSvg
    LIBS += -framework QtQuick
    LIBS += -framework QtQuick3D
    LIBS += -framework QtQuick3DUtils
    LIBS += -framework QtQuick3DRuntimeRender
    LIBS += -framework QtQuickShapes
    LIBS += -framework QtQml
    LIBS += -framework QtQmlMeta
    LIBS += -framework QtQmlModels
    LIBS += -framework QtQmlWorkerScript
    LIBS += -framework QtGui
    LIBS += -framework QtWidgets

    # Added to Xcode project
    qtconf.files = $$PWD/qt.conf
    qtconf.path = Contents/Resources
    QMAKE_BUNDLE_DATA += qtconf

    # Added to Xcode project
    LICENSE.files = $$PWD/LICENSE
    LICENSE.path = Contents/Resources
    QMAKE_BUNDLE_DATA += LICENSE

    # Added to Xcode project
    privacy.files = $$PWD/privacy.txt
    privacy.path = Contents/Resources
    QMAKE_BUNDLE_DATA += privacy

    # Added to Xcode project
    platforms.files = \
        $$QTDIR/plugins/platforms/libqoffscreen.dylib \
        $$QTDIR/plugins/platforms/libqminimal.dylib \
        $$QTDIR/plugins/platforms/libqcocoa.dylib
    platforms.path = Contents/PlugIns/platforms
    QMAKE_BUNDLE_DATA += platforms

    # Added to Xcode project
    canbus.files = \
        $$QTDIR/plugins/canbus/libqtpassthrucanbus.dylib \
        $$QTDIR/plugins/canbus/libqtpeakcanbus.dylib \
        $$QTDIR/plugins/canbus/libqttinycanbus.dylib \
        $$QTDIR/plugins/canbus/libqtvirtualcanbus.dylib
    canbus.path = Contents/PlugIns/canbus
    QMAKE_BUNDLE_DATA += canbus

    # Added to Xcode project
    generic.files = \
        $$QTDIR/plugins/generic/libqinsighttracker.dylib \
        $$QTDIR/plugins/generic/libqtuiotouchplugin.dylib
    generic.path = Contents/PlugIns/canbus
    QMAKE_BUNDLE_DATA += generic

    # Added to Xcode project
    geometryloaders.files = \
        $$QTDIR/plugins/geometryloaders/libdefaultgeometryloader.dylib \
        $$QTDIR/plugins/geometryloaders/libgltfgeometryloader.dylib
    geometryloaders.path = Contents/PlugIns/geometryloaders
    QMAKE_BUNDLE_DATA += geometryloaders

    # Added to Xcode project
    geoservices.files = \
        $$QTDIR/plugins/geoservices/libqtgeoservices_itemsoverlay.dylib \
        $$QTDIR/plugins/geoservices/libqtgeoservices_osm.dylib
    geoservices.path = Contents/PlugIns/geoservices
    QMAKE_BUNDLE_DATA += geoservices

    # Added to Xcode project
    help.files = \
        $$QTDIR/plugins/help/libhelpplugin.dylib
    help.path = Contents/PlugIns/help
    QMAKE_BUNDLE_DATA += help

    # Added to Xcode project
    iconengines.files = \
        $$QTDIR/plugins/iconengines/libqsvgicon.dylib
    iconengines.path = Contents/PlugIns/iconengines
    QMAKE_BUNDLE_DATA += iconengines

    # Added to Xcode project
    imageformats.files = \
        $$QTDIR/plugins/imageformats/libqgif.dylib \
        $$QTDIR/plugins/imageformats/libqicns.dylib \
        $$QTDIR/plugins/imageformats/libqico.dylib \
        $$QTDIR/plugins/imageformats/libqjpeg.dylib \
        $$QTDIR/plugins/imageformats/libqmacheif.dylib \
        $$QTDIR/plugins/imageformats/libqmacjp2.dylib \
        $$QTDIR/plugins/imageformats/libqpdf.dylib \
        $$QTDIR/plugins/imageformats/libqsvg.dylib \
        $$QTDIR/plugins/imageformats/libqtga.dylib \
        $$QTDIR/plugins/imageformats/libqtiff.dylib \
        $$QTDIR/plugins/imageformats/libqwbmp.dylib \
        $$QTDIR/plugins/imageformats/libqwebp.dylib
    imageformats.path = Contents/PlugIns/imageformats
    QMAKE_BUNDLE_DATA += imageformats

    # Added to Xcode project
    networkinformation.files = \
        $$QTDIR/plugins/networkinformation/libqscnetworkreachability.dylib
    networkinformation.path = Contents/PlugIns/networkinformation
    QMAKE_BUNDLE_DATA += networkinformation

    # Added to Xcode project
    platforminputcontexts.files = \
        $$QTDIR/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.dylib
    platforminputcontexts.path = Contents/PlugIns/platforminputcontexts
    QMAKE_BUNDLE_DATA += platforminputcontexts

    # Added to Xcode project
    renderers.files = \
        $$QTDIR/plugins/renderers/libopenglrenderer.dylib \
        $$QTDIR/plugins/renderers/librhirenderer.dylib
    renderers.path = Contents/PlugIns/renderers
    QMAKE_BUNDLE_DATA += renderers

    # Added to Xcode project
    renderplugins.files = \
        $$QTDIR/plugins/renderplugins/libscene2d.dylib
    renderplugins.path = Contents/PlugIns/renderplugins
    QMAKE_BUNDLE_DATA += renderplugins

    # Added to Xcode project
    sceneparsers.files = \
        $$QTDIR/plugins/sceneparsers/libassimpsceneimport.dylib \
        $$QTDIR/plugins/sceneparsers/libgltfsceneexport.dylib \
        $$QTDIR/plugins/sceneparsers/libgltfsceneimport.dylib
    sceneparsers.path = Contents/PlugIns/sceneparsers
    QMAKE_BUNDLE_DATA += sceneparsers

    # Added to Xcode project
    scxmldatamodel.files = \
        $$QTDIR/plugins/scxmldatamodel/libqscxmlecmascriptdatamodel.dylib
    scxmldatamodel.path = Contents/PlugIns/scxmldatamodel
    QMAKE_BUNDLE_DATA += scxmldatamodel

    # Added to Xcode project
    sensors.files = \
        $$QTDIR/plugins/sensors/libqtsensors_generic.dylib
    sensors.path = Contents/PlugIns/sensors
    QMAKE_BUNDLE_DATA += sensors

    # Added to Xcode project
    styles.files = \
        $$QTDIR/plugins/styles/libqmacstyle.dylib
    styles.path = Contents/PlugIns/styles
    QMAKE_BUNDLE_DATA += styles

    # Added to Xcode project
    texttospeech.files = \
        $$QTDIR/plugins/texttospeech/libqtexttospeech_mock.dylib \
        $$QTDIR/plugins/texttospeech/libqtexttospeech_speechdarwin.dylib
    texttospeech.path = Contents/PlugIns/texttospeech
    QMAKE_BUNDLE_DATA += texttospeech

    # Added to Xcode project
    tls.files = \
        $$QTDIR/plugins/tls/libqcertonlybackend.dylib \
        $$QTDIR/plugins/tls/libqopensslbackend.dylib \
        $$QTDIR/plugins/tls/libqsecuretransportbackend.dylib
    tls.path = Contents/PlugIns/tls
    QMAKE_BUNDLE_DATA += tls

    # Added to Xcode project
    frameworks.files = \
        $$QTDIR/lib/QtCore.framework \
        $$QTDIR/lib/QtConcurrent.framework \
        $$QTDIR/lib/QtNetwork.framework \
        $$QTDIR/lib/QtDBus.framework \
        $$QTDIR/lib/QtCore5Compat.framework \
        $$QTDIR/lib/QtOpenGL.framework \
        $$QTDIR/lib/QtShaderTools.framework \
        $$QTDIR/lib/QtSvg.framework \
        $$QTDIR/lib/QtGraphs.framework \
        $$QTDIR/lib/QtGui.framework \
        $$QTDIR/lib/QtWidgets.framework \
        $$QTDIR/lib/QtQuick.framework \
        $$QTDIR/lib/QtQuickShapes.framework \
        $$QTDIR/lib/QtQuick3D.framework \
        $$QTDIR/lib/QtQuick3DRuntimeRender.framework \
        $$QTDIR/lib/QtQuick3DUtils.framework \
        $$QTDIR/lib/QtQml.framework  \
        $$QTDIR/lib/QtQmlMeta.framework \
        $$QTDIR/lib/QtQmlModels.framework \
        $$QTDIR/lib/QtQmlWorkerScript.framework
    frameworks.path = Contents/Frameworks
    QMAKE_BUNDLE_DATA += frameworks

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
        $$PWD/assets/icns/RoccatTyon_64x64.icns \
        $$PWD/assets/icns/RoccatTyon.icns
        icons.path = Contents/Resources/
    QMAKE_BUNDLE_DATA += icons
}

SOURCES += \
    main.cpp \
    rtabstractdevice.cpp \
    rtcalibratetcudialog.cpp \
    rtcalibratexcdialog.cpp \
    rtcolordialog.cpp \
    rtcontroller.cpp \
    rthidmacos.cpp \
    rtmainwindow.cpp \
    rtprogress.cpp \
    rtshortcutdialog.cpp \
    rttablemodel.cpp \
    rttcuimagewidget.cpp \
    rtxceleratorwidget.cpp

HEADERS += \
    hid_uid.h \
    rtabstractdevice.h \
    rtcalibratetcudialog.h \
    rtcalibratexcdialog.h \
    rtcolordialog.h \
    rtcontroller.h \
    rthiddevicedbg.hpp \
    rthidmacos.h \
    rtmainwindow.h \
    rtprogress.h \
    rtshortcutdialog.h \
    rttablemodel.h \
    rttcuimagewidget.h \
    rttypedefs.h \
    rtxceleratorwidget.h

FORMS += \
    rtcalibratetcudialog.ui \
    rtcalibratexcdialog.ui \
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
    make.sh \
    roccattyon.qmodel
