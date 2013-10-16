project(muon)
cmake_minimum_required(VERSION 2.8)

macro(kde4_add_library)
    add_library(${ARGN})
endmacro()
macro(kde4_add_executable)
    add_executable(${ARGN})
endmacro()
macro(kde4_add_test)
    add_executable(${ARGN})
endmacro()

macro(kde4_add_kcfg_files)
endmacro()

macro(kde4_add_ui_files)
endmacro()

macro(qt4_add_resources)
endmacro()

macro(kde4_install_icons)
endmacro()

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt5 5.2.0 REQUIRED NO_MODULE COMPONENTS Widgets Test Network Xml Concurrent DBus Svg)
find_package(KF5 REQUIRED MODULE COMPONENTS CMake Compiler InstallDirs)

find_package(KCoreAddons REQUIRED NO_MODULE)
find_package(KConfig REQUIRED NO_MODULE)
find_package(KI18n REQUIRED NO_MODULE)
find_package(KDBusAddons REQUIRED NO_MODULE)
find_package(ItemViews REQUIRED NO_MODULE)
find_package(KCodecs REQUIRED NO_MODULE)
find_package(KAuth REQUIRED NO_MODULE)
find_package(KGuiAddons REQUIRED NO_MODULE)
find_package(KWidgetsAddons REQUIRED NO_MODULE)
find_package(KJobWidgets REQUIRED NO_MODULE)
find_package(KConfigWidgets REQUIRED NO_MODULE)
find_package(KIconThemes REQUIRED NO_MODULE)
find_package(KWindowSystem REQUIRED NO_MODULE)
find_package(KNotifications REQUIRED NO_MODULE)
find_package(KCompletion REQUIRED NO_MODULE)
find_package(KService REQUIRED NO_MODULE)
find_package(KIO REQUIRED NO_MODULE)
find_package(Solid REQUIRED NO_MODULE)
find_package(KUnitConversion REQUIRED NO_MODULE)
find_package(Sonnet REQUIRED NO_MODULE)
find_package(kwidgets REQUIRED NO_MODULE)
find_package(KTextWidgets REQUIRED NO_MODULE)
find_package(KArchive REQUIRED NO_MODULE)
find_package(KJS REQUIRED NO_MODULE)

find_package(KDELibs4 REQUIRED NO_MODULE)




# find_package(QApt QUIET)
# find_package(DebconfKDE QUIET)
# find_package(QtOAuth REQUIRED)
# find_package(QCA2 REQUIRED) #required by QtOAuth
# find_package(QJSON REQUIRED)
# find_package(LibAttica QUIET)
# find_package(BODEGA QUIET)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${QAPT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR} ${QCA2_INCLUDE_DIR}
                    ${QJSON_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_BINARY_DIR}/libmuon)

add_subdirectory(libmuon)
if(QAPT_FOUND)
	add_subdirectory(libmuonapt)
    add_subdirectory(muon)
    add_subdirectory(installer)
    add_subdirectory(kded)
endif(QAPT_FOUND)
add_subdirectory(updater)
add_subdirectory(discover)
add_subdirectory(exporter)

# macro_log_feature(QAPT_FOUND "LibQApt" "Qt wrapper around the libapt-pkg library"
#                   "http://www.kde.org" FALSE "" "Required to build the APT backend")
# macro_log_feature(LIBATTICA_FOUND "LibAttica" "Qt library that implements the Open Collaboration Services API"
#                   "http://www.kde.org" FALSE "" "Required to build the KNewStuff3 backend")
# macro_log_feature(BODEGA_FOUND "Bodega" "Library that exposes Bodega resources"
#                   "http://www.kde.org" FALSE "" "Required to build the Bodega backend")

# macro_display_feature_log()
