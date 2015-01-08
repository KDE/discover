project(muon)
set(PROJECT_VERSION "5.1.95")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12)

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt5 5.2.0 REQUIRED CONFIG COMPONENTS Widgets Test Network Xml Concurrent DBus Svg Qml QuickWidgets)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(GenerateExportHeader)

find_package(KF5 REQUIRED WidgetsAddons CoreAddons Solid
                                            Config Wallet DBusAddons I18n ConfigWidgets Notifications Declarative)

find_package(Phonon4Qt5 REQUIRED NO_MODULE)

find_package(QApt 3.0.0)
if(QApt_FOUND)
    find_package(DebconfKDE 1.0.0 REQUIRED)
    find_package(QtOAuth REQUIRED)
    find_package(Qca-qt5 REQUIRED) #required by QtOAuth
endif()
find_package(KF5Attica)
find_package(KF5NewStuff)
# find_package(BODEGA QUIET)
find_package(AkabeiClient QUIET)
find_package(packagekitqt5)
if(packagekitqt5_FOUND)
    find_package(AppstreamQt)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(MUON_VERSION ${PROJECT_VERSION})
configure_file(MuonVersion.h.in MuonVersion.h)

add_subdirectory(libmuon)
if(QApt_FOUND)
    add_subdirectory(libmuonapt)
    add_subdirectory(muon)
#    add_subdirectory(installer)
endif()
add_subdirectory(updater)
add_subdirectory(discover)
add_subdirectory(exporter)
add_subdirectory(notifier)

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
