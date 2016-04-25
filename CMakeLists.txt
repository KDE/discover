project(discover)
set(PROJECT_VERSION "5.6.3")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12)

option(INSTALL_EXPORTER "Install plasma-discover-exporter [development utility]" OFF)

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5 5.2.0 REQUIRED CONFIG COMPONENTS Widgets Test Network Xml Concurrent DBus Quick)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(GenerateExportHeader)

find_package(KF5 REQUIRED CoreAddons Config Crash DBusAddons I18n Declarative)
find_package(KF5TextWidgets REQUIRED)

find_package(packagekitqt5)
if (NOT packagekitqt5_FOUND)
    find_package(QApt 3.0.0)
    if(QApt_FOUND)
        find_package(DebconfKDE 1.0.0 REQUIRED)
        find_package(KF5 REQUIRED Archive IconThemes Notifications KIO)
    endif()
endif()
find_package(AppstreamQt 0.9.2)
find_package(KF5Attica)
find_package(KF5NewStuff)
# find_package(BODEGA QUIET)

if (${KF5_VERSION} VERSION_GREATER "5.14.0")
    add_definitions(-DWITH_KCRASH_INIT)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

configure_file(DiscoverVersion.h.in DiscoverVersion.h)

add_subdirectory(libdiscover)
add_subdirectory(updater)
add_subdirectory(discover)
add_subdirectory(exporter)

option(WITH_NOTIFIER "Build and install the notifier plasmoid" ON)
if(WITH_NOTIFIER)
    find_package(KF5 REQUIRED Notifications KIO)
    add_subdirectory(notifier)
endif()

set_package_properties(QApt PROPERTIES
    DESCRIPTION "Qt wrapper around the libapt-pkg library"
    PURPOSE "Used to support apt-based distribution systems"
    TYPE OPTIONAL)

set_package_properties(KF5Attica PROPERTIES
    DESCRIPTION "KDE Framework that implements the Open Collaboration Services API"
    PURPOSE "Required to build the KNewStuff3 backend"
    TYPE OPTIONAL)
set_package_properties(KF5NewStuff PROPERTIES
    DESCRIPTION "Qt library that allows to interact with KNewStuff implementations"
    PURPOSE "Required to build the KNS backend"
    TYPE OPTIONAL)
set_package_properties(Bodega PROPERTIES
    DESCRIPTION "Library that exposes Bodega resources"
    PURPOSE "Required to build the Bodega backend"
    TYPE OPTIONAL)
set_package_properties(AkabeiClient PROPERTIES
    DESCRIPTION "Library that exposes Akabei resources"
    PURPOSE "Required to build the Akabei backend (Supports Chakra distribution)"
    TYPE OPTIONAL)
set_package_properties(packagekitqt5 PROPERTIES
    DESCRIPTION "Library that exposes PackageKit resources"
    URL "http://www.packagekit.org"
    PURPOSE "Required to build the PackageKit backend"
    TYPE OPTIONAL)
set_package_properties(AppstreamQt PROPERTIES
    DESCRIPTION "Library that lists Appstream resources"
    URL "http://www.freedesktop.org"
    PURPOSE "Required to build the PackageKit backend"
    TYPE OPTIONAL)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
