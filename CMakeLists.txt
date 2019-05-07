project(discover)
set(PROJECT_VERSION "5.15.5")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12)

set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.54.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets Test Network Xml Concurrent DBus Quick)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMAddTests)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)

find_package(PkgConfig REQUIRED)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED CoreAddons Config Crash DBusAddons I18n Archive XmlGui ItemModels KIO Declarative)
find_package(KF5Kirigami2 2.1.0)

find_package(packagekitqt5 1.0.1 CONFIG)
find_package(AppStreamQt 0.11.1 CONFIG)
find_package(KF5Attica 5.23 CONFIG)
find_package(KF5NewStuff 5.53 CONFIG)

set(CMAKE_AUTORCC ON)

pkg_check_modules(Flatpak IMPORTED_TARGET flatpak>=0.11.8)
pkg_check_modules(Fwupd IMPORTED_TARGET fwupd>=1.0.6)
pkg_check_modules(Markdown IMPORTED_TARGET libmarkdown)

if(NOT CMAKE_VERSION VERSION_LESS "3.10.0")
  # CMake 3.9+ warns about automoc on files without Q_OBJECT, and doesn't know about other macros.
  # 3.10+ lets us provide more macro names that require automoc.
  list(APPEND CMAKE_AUTOMOC_MACRO_NAMES "DISCOVER_BACKEND_PLUGIN")
endif()

configure_file(DiscoverVersion.h.in DiscoverVersion.h)

add_subdirectory(libdiscover)
add_subdirectory(discover)
add_subdirectory(exporter)

option(WITH_NOTIFIER "Build and install the notifier plasmoid" ON)
if(WITH_NOTIFIER)
    find_package(KF5 REQUIRED Notifications KIO)
    add_subdirectory(notifier)
endif()

set_package_properties(KF5Attica PROPERTIES
    DESCRIPTION "KDE Framework that implements the Open Collaboration Services API"
    PURPOSE "Required to build the KNewStuff3 backend"
    TYPE OPTIONAL)
set_package_properties(KF5Kirigami2 PROPERTIES
    DESCRIPTION "KDE's lightweight user interface framework for mobile and convergent applications"
    URL "https://techbase.kde.org/Kirigami"
    PURPOSE "Required by discover qml components"
    TYPE RUNTIME)
set_package_properties(KF5NewStuff PROPERTIES
    DESCRIPTION "Qt library that allows to interact with KNewStuff implementations"
    PURPOSE "Required to build the KNS backend"
    TYPE OPTIONAL)
set_package_properties(packagekitqt5 PROPERTIES
    DESCRIPTION "Library that exposes PackageKit resources"
    URL "http://www.packagekit.org"
    PURPOSE "Required to build the PackageKit backend"
    TYPE OPTIONAL)
set_package_properties(AppStreamQt PROPERTIES
    DESCRIPTION "Library that lists Appstream resources"
    URL "http://www.freedesktop.org"
    PURPOSE "Required to build the PackageKit and Flatpak backends"
    TYPE OPTIONAL)
set_package_properties(FLATPAK PROPERTIES
    DESCRIPTION "Library that exposes flatpak repositories"
    URL "http://www.freedesktop.org"
    PURPOSE "Required to build the Flatpak backend"
    TYPE OPTIONAL)
set_package_properties(LIBFWUPD PROPERTIES
    DESCRIPTION "Library that exposes fwupd"
    URL "http://www.fwupd.org"
    PURPOSE "Required to build the Fwupd backend"
    TYPE OPTIONAL)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

install( FILES discover.categories DESTINATION ${KDE_INSTALL_CONFDIR} )
