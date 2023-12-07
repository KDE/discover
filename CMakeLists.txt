cmake_minimum_required(VERSION 3.16)

project(discover)
set(PROJECT_VERSION "5.90.90")
set(PROJECT_VERSION_MAJOR 6)

set(QT_MIN_VERSION "6.5.0")
set(KF6_MIN_VERSION "5.240.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

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
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED CONFIG COMPONENTS Widgets Test Network Concurrent DBus Quick)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS WebView)

find_package(PkgConfig REQUIRED)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED Kirigami CoreAddons Config Crash DBusAddons I18n Archive XmlGui KIO KCMUtils IdleTime Notifications Purpose Auth StatusNotifierItem)
find_package(KF6 ${KF6_MIN_VERSION} COMPONENTS UserFeedback)

find_package(packagekitqt6 1.0.1 CONFIG)
find_package(AppStreamQt 1.0 CONFIG REQUIRED)
find_package(KF6Attica 5.23 CONFIG)
find_package(KF6NewStuff 5.53 CONFIG)

find_package(KF${QT_MAJOR_VERSION}KirigamiAddons REQUIRED)
set_package_properties(KF${QT_MAJOR_VERSION}KirigamiAddons PROPERTIES
    TYPE REQUIRED
    PURPOSE "Provides additional visual components"
)

pkg_check_modules(Flatpak IMPORTED_TARGET flatpak>=0.11.8)
pkg_check_modules(Fwupd IMPORTED_TARGET fwupd>=1.9.4)
pkg_check_modules(Markdown IMPORTED_TARGET libmarkdown)
if(Markdown_FOUND AND Markdown_VERSION VERSION_GREATER_EQUAL 3)
    add_definitions(-DMARKDOWN3)
endif()
pkg_check_modules(Ostree IMPORTED_TARGET ostree-1)
pkg_check_modules(RpmOstree IMPORTED_TARGET rpm-ostree-1)

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

set_package_properties(KF6Attica PROPERTIES
    DESCRIPTION "KDE Framework that implements the Open Collaboration Services API"
    PURPOSE "Required to build the KNewStuff3 backend"
    TYPE OPTIONAL)
set_package_properties(KF6Kirigami2 PROPERTIES
    DESCRIPTION "KDE's lightweight user interface framework for mobile and convergent applications"
    URL "https://techbase.kde.org/Kirigami"
    PURPOSE "Required by discover qml components"
    TYPE RUNTIME)
set_package_properties(KF6NewStuff PROPERTIES
    DESCRIPTION "Qt library that allows to interact with KNewStuff implementations"
    PURPOSE "Required to build the KNS backend"
    TYPE OPTIONAL)
set_package_properties(KF6UserFeedback PROPERTIES
    DESCRIPTION "Framework for collecting feedback from application users via telemetry and targeted surveys"
    URL "https://invent.kde.org/frameworks/kuserfeedback"
    PURPOSE "Required for submission of telemetry data"
    TYPE OPTIONAL)
set_package_properties(packagekitqt6 PROPERTIES
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
