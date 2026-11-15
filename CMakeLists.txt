# plugins/Backgammon/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(BackgammonPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED QT_VERSION_MAJOR)
    find_package(Qt6 QUIET COMPONENTS Core)
    if(Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    else()
        find_package(Qt5 COMPONENTS Core REQUIRED)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Multimedia REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BACKGAMMON_SOURCES
    BackgammonPlugin.cpp
    core/BgRules.cpp
    core/BgMatch.cpp
    services/BackgammonService.cpp
    gui/BackgammonPage.cpp
    gui/BackgammonGameWindow.cpp
    gui/BackgammonSettingsPage.cpp
    gui/BackgammonSettingsWidget.cpp
    gui/BackgammonSettingsDialog.cpp
    gui/BackgammonResultDialog.cpp
    gui/BackgammonTrafficDialog.cpp
    gui/BackgammonSettings.cpp
    gui/BackgammonSounds.cpp
    gui/BackgammonUserNotify.cpp
    gui/BoardTheme.cpp
    gui/BoardWidget.cpp
    gui/toaster/BackgammonToaster.cpp
    gui/toaster/BackgammonToasterNotify.cpp
)

set(BACKGAMMON_HEADERS
    BackgammonPlugin.h
    core/BgRules.h
    core/BgMatch.h
    services/BackgammonService.h
    services/BackgammonDebug.h
    gui/BackgammonConstants.h
    gui/BackgammonPage.h
    gui/BackgammonGameWindow.h
    gui/BackgammonSettingsPage.h
    gui/BackgammonSettingsWidget.h
    gui/BackgammonSettingsDialog.h
    gui/BackgammonResultDialog.h
    gui/BackgammonTrafficDialog.h
    gui/BackgammonSettings.h
    gui/BackgammonSounds.h
    gui/BackgammonStyle.h
    gui/BackgammonUserNotify.h
    gui/BoardTheme.h
    gui/BoardWidget.h
    gui/toaster/BackgammonToaster.h
    gui/toaster/BackgammonToasterNotify.h
)

set(BACKGAMMON_FORMS
    gui/BackgammonPage.ui
    gui/BackgammonGameWindow.ui
    gui/BackgammonSettingsPage.ui
    gui/BackgammonSettingsWidget.ui
    gui/BackgammonSettingsDialog.ui
    gui/BackgammonResultDialog.ui
    gui/BackgammonTrafficDialog.ui
    gui/InvitationItem.ui
    gui/toaster/BackgammonToaster.ui
)

set(BACKGAMMON_RESOURCES
    gui/Backgammon_images.qrc
    gui/Backgammon_sounds.qrc
    gui/Backgammon_qss.qrc
)

add_library(Backgammon MODULE
    ${BACKGAMMON_SOURCES}
    ${BACKGAMMON_HEADERS}
    ${BACKGAMMON_FORMS}
    ${BACKGAMMON_RESOURCES}
)

set_target_properties(Backgammon PROPERTIES
    PREFIX "lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/gui;${CMAKE_CURRENT_SOURCE_DIR}/gui/toaster"
)

# Works both from the super-project (add_subdirectory) and standalone, as
# long as the plugin lives in <root>/plugins/Backgammon.
get_filename_component(RS_SUPERPROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

if(WIN32)
    target_compile_definitions(Backgammon PRIVATE WINDOWS_SYS)

    # A Windows DLL resolves every symbol at link time, so it links against
    # RetroShare itself: the CMake targets in the super-project, or the import
    # libraries of a RetroShare build tree when built standalone.
    if(TARGET retroshare AND TARGET retroshare-gui)
        target_link_libraries(Backgammon PRIVATE retroshare retroshare-gui)
    else()
        set(RS_BUILD_DIR "${RS_SUPERPROJECT_ROOT}/Build-cmake" CACHE PATH
            "RetroShare build directory to link against when building standalone")

        find_library(RS_LIBRETROSHARE_IMPLIB NAMES retroshare
                     HINTS "${RS_BUILD_DIR}/libretroshare" NO_DEFAULT_PATH)
        find_library(RS_GUI_IMPLIB NAMES retroshare-gui
                     HINTS "${RS_BUILD_DIR}/retroshare-gui" NO_DEFAULT_PATH)

        if(NOT RS_LIBRETROSHARE_IMPLIB OR NOT RS_GUI_IMPLIB)
            message(FATAL_ERROR
                "Standalone Windows build needs the import libraries of a built "
                "RetroShare. Looked in ${RS_BUILD_DIR} for libretroshare.dll.a and "
                "libretroshare-gui.dll.a. Build RetroShare first, or point "
                "-DRS_BUILD_DIR=<its build directory> at it.")
        endif()

        message(STATUS "Backgammon: standalone Windows link against ${RS_BUILD_DIR}")
        target_link_libraries(Backgammon PRIVATE
            "${RS_LIBRETROSHARE_IMPLIB}" "${RS_GUI_IMPLIB}")
    endif()
endif()

target_link_libraries(Backgammon PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

target_compile_definitions(Backgammon PRIVATE RS_NO_WARN_DEPRECATED)

# Must match the host build: RS_USE_CALENDAR adds a member to RsPlugInInterfaces
# (retroshare/rsplugin.h) before mGxsTunnels, so a plugin built without it reads
# the wrong pointer and crashes at startup. ON by default, like libretroshare.
option(RS_USE_CALENDAR "Build against a RetroShare with the GXS Calendar service (default in CMake builds)" ON)
if(RS_USE_CALENDAR)
    target_compile_definitions(Backgammon PRIVATE RS_USE_CALENDAR)
endif()

target_compile_options(Backgammon PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-Wno-deprecated-declarations>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,15>>:-Wno-template-body>
)

if(RS_DEVELOPMENT_BUILD)
    target_compile_options(Backgammon PRIVATE $<$<CXX_COMPILER_ID:GNU>:-g;-O0>)
endif()

target_include_directories(Backgammon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RS_SUPERPROJECT_ROOT}/libretroshare/src
    ${RS_SUPERPROJECT_ROOT}/retroshare-gui/src
    ${RS_SUPERPROJECT_ROOT}/retroshare-gui/src/gui
    ${RS_SUPERPROJECT_ROOT}/supportlibs/rapidjson/include
)

install(TARGETS Backgammon DESTINATION lib/retroshare/plugins)

# Tests of the rules core (no Qt, no RetroShare): ctest after a build.
option(BACKGAMMON_BUILD_TESTS "Build the tests of the Backgammon rules core" OFF)
if(BACKGAMMON_BUILD_TESTS)
    enable_testing()
    add_executable(backgammon-core-test tests/core-test.cpp core/BgRules.cpp core/BgMatch.cpp)
    target_include_directories(backgammon-core-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)
    add_test(NAME backgammon-core COMMAND backgammon-core-test)
endif()
