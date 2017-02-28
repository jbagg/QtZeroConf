QT = core network

include($$PWD/qtzeroconf.pri)

#VERSION = 1.0

TEMPLATE = lib
TARGET = $$qtLibraryTarget(QtZeroConf$$QT_LIBINFIX)
CONFIG += module create_prl
DEFINES+= QT_BUILD_ZEROCONF_LIB
mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET

target.path = $$PREFIX/lib

INSTALLS += target
