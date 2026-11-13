################################################################################
# Backgammon.pro                                                               #
# Copyright (C) 2026, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                             #
################################################################################

!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

# Generated files must never be shared between Qt major versions or build modes.
CONFIG(debug, debug|release) {
	BACKGAMMON_BUILD_MODE = debug
} else {
	BACKGAMMON_BUILD_MODE = release
}
BACKGAMMON_GENERATED_DIR = $$OUT_PWD/temp/qt$${QT_MAJOR_VERSION}/$${BACKGAMMON_BUILD_MODE}
OBJECTS_DIR = $$BACKGAMMON_GENERATED_DIR/obj
MOC_DIR = $$BACKGAMMON_GENERATED_DIR/moc
RCC_DIR = $$BACKGAMMON_GENERATED_DIR/qrc
UI_DIR = $$BACKGAMMON_GENERATED_DIR/ui

QT += widgets multimedia
CONFIG += qt uic qrc resources

CONFIG += c++17

DESTDIR = lib
TARGET = Backgammon

# $$PWD for "core/...", "gui/..." and "services/..."
INCLUDEPATH += $$PWD
DEPENDPATH  += $$PWD

DEPENDPATH  += ../../libretroshare/src ../../retroshare-gui/src/temp/ui
INCLUDEPATH += ../../libretroshare/src ../../retroshare-gui/src/temp/ui
INCLUDEPATH += ../../retroshare-gui/src ../../retroshare-gui/src/retroshare-gui
INCLUDEPATH += ../../rapidjson-1.1.0

QMAKE_CXXFLAGS *= -Wall

# RapidJSON 1.1.0 contains an assignment operator that modifies const members.
# GCC 15 checks this eagerly in template bodies, even when it is never used.
greaterThan(QMAKE_GCC_MAJOR_VERSION, 14) {
	QMAKE_CXXFLAGS += -Wno-template-body
}

################################### SOURCES ###################################

SOURCES = BackgammonPlugin.cpp \
          core/BgRules.cpp \
          core/BgMatch.cpp \
          services/BackgammonService.cpp \
          gui/BackgammonPage.cpp \
          gui/BackgammonGameWindow.cpp \
          gui/BackgammonSettingsPage.cpp \
          gui/BackgammonSounds.cpp \
          gui/BoardTheme.cpp \
          gui/BoardWidget.cpp \
          gui/BackgammonSettings.cpp \
          gui/BackgammonUserNotify.cpp \
          gui/toaster/BackgammonToaster.cpp \
          gui/toaster/BackgammonToasterNotify.cpp

HEADERS = BackgammonPlugin.h \
          core/BgRules.h \
          core/BgMatch.h \
          services/BackgammonService.h \
          gui/BackgammonConstants.h \
          gui/BackgammonPage.h \
          gui/BackgammonGameWindow.h \
          gui/BackgammonSettingsPage.h \
          gui/BackgammonSounds.h \
          gui/BoardTheme.h \
          gui/BoardWidget.h \
          gui/BackgammonSettings.h \
          gui/BackgammonUserNotify.h \
          gui/toaster/BackgammonToaster.h \
          gui/toaster/BackgammonToasterNotify.h

FORMS = gui/BackgammonPage.ui \
        gui/BackgammonGameWindow.ui \
        gui/BackgammonSettingsPage.ui \
        gui/toaster/BackgammonToaster.ui \
        gui/InvitationItem.ui

RESOURCES = gui/Backgammon_images.qrc \
            gui/Backgammon_sounds.qrc

# The rules core has its own tests, built without Qt:
#   g++ -std=c++17 -I core tests/core-test.cpp core/BgRules.cpp core/BgMatch.cpp -o core-test
