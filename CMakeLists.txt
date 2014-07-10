project(muon)
cmake_minimum_required(VERSION 2.8.12)

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 5.2.0 REQUIRED CONFIG COMPONENTS Widgets Test Network Xml Concurrent DBus Svg Qml QuickWidgets)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(ECMMarkAsTest)

find_package(KF5 REQUIRED IdleTime ItemModels WidgetsAddons WindowSystem Codecs Archive CoreAddons Solid ThreadWeaver GlobalAccel
                                            Config Auth JS Wallet DBusAddons I18n GuiAddons ConfigWidgets
                                            Service ItemViews Notifications IconThemes Completion JobWidgets Sonnet TextWidgets XmlGui Crash
                                            Bookmarks Declarative UnitConversion Parts KIO DNSSD KDELibs4Support)

find_package(Phonon4Qt5 REQUIRED NO_MODULE)

find_package(QApt QUIET)
find_package(DebconfKDE QUIET)
# find_package(QtOAuth REQUIRED)
# find_package(QCA2 REQUIRED) #required by QtOAuth
# find_package(LibAttica QUIET)
# find_package(BODEGA QUIET)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(MUON_VERSION "2.65.0")
configure_file(MuonVersion.h.in MuonVersion.h)

add_subdirectory(libmuon)
if(QAPT_FOUND)
    add_subdirectory(libmuonapt)
    add_subdirectory(muon)
    add_subdirectory(installer)
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
# macro_log_feature(AKABEICLIENT_FOUND "Akabei" "Library that exposes Akabei resources"
#                   "http://www.kde.org" FALSE "" "Required to build the Akabei backend")
# macro_log_feature(PACKAGEKITQT2_FOUND "PackageKitQt2" "Library that exposes PackageKit resources"
#                   "http://www.packagekit.org" FALSE "" "Required to build the PackageKit backend")
# macro_display_feature_log()
