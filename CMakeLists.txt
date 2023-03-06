project(discover)
set(PROJECT_VERSION "5.27.80")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 3.16)

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.102.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH}  "${CMAKE_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMAddTests)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(KDEClangFormat)
include(KDEGitCommitHooks)
find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets Test Network Concurrent DBus Quick)
find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS WebView)

find_package(PkgConfig REQUIRED)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED CoreAddons Config Crash DBusAddons I18n Archive XmlGui KIO Declarative KCMUtils IdleTime Notifications Purpose)
find_package(KF5Kirigami2 2.7.0)

find_package(packagekitqt5 1.0.1 CONFIG)
find_package(AppStreamQt 0.15.3 CONFIG REQUIRED)
find_package(KF5Attica 5.23 CONFIG)
find_package(KF5NewStuff 5.53 CONFIG)

pkg_check_modules(Flatpak IMPORTED_TARGET flatpak>=0.11.8)
pkg_check_modules(Fwupd IMPORTED_TARGET fwupd>=1.5.0)
pkg_check_modules(Markdown IMPORTED_TARGET libmarkdown)
pkg_check_modules(Ostree IMPORTED_TARGET ostree-1)
pkg_check_modules(RpmOstree IMPORTED_TARGET rpm-ostree-1)
find_package(KUserFeedback)

list(APPEND CMAKE_AUTOMOC_MACRO_NAMES "DISCOVER_BACKEND_PLUGIN")

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS -DQT_NO_URL_CAST_FROM_STRING)

configure_file(DiscoverVersion.h.in DiscoverVersion.h)

add_subdirectory(appiumtests)
add_subdirectory(libdiscover)
add_subdirectory(discover)
add_subdirectory(exporter)
add_subdirectory(update)

option(WITH_KCM "Build and install the updates KCM" ON)
if(WITH_KCM)
    add_subdirectory(kcm)
endif()

option(WITH_NOTIFIER "Build and install the notifier plasmoid" ON)
if(WITH_NOTIFIER)
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
    URL "https://www.freedesktop.org/software/PackageKit/"
    PURPOSE "Required to build the PackageKit backend"
    TYPE OPTIONAL)
set_package_properties(AppStreamQt PROPERTIES
    DESCRIPTION "Library that lists Appstream resources"
    URL "https://www.freedesktop.org"
    PURPOSE "Required to build the PackageKit, Flatpak and Snap backends"
    TYPE OPTIONAL)
add_feature_info(Flatpak Flatpak_FOUND
                "Library that exposes flatpak repositories. Required to build the Flatpak backend"
)
add_feature_info(Fwupd Fwupd_FOUND "Exposes fwupd")
add_feature_info(Ostree Ostree_FOUND
                "Library to manage ostree repository. Required to build the rpm-ostree backend"
)
add_feature_info(RpmOstree RpmOstree_FOUND
                "rpm-ostree binary to manage the system. Required to build the rpm-ostree backend"
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ecm_qt_install_logging_categories(
        EXPORT DISCOVER
        FILE discover.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

ki18n_install(po)
