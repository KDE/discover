project(muon)
cmake_minimum_required(VERSION 2.8)

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 5.2.0 REQUIRED CONFIG COMPONENTS Widgets Test Network Xml Concurrent DBus Svg Declarative Qml Quick)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)

find_package(KF5 CONFIG REQUIRED IdleTime ItemModels WidgetsAddons WindowSystem Codecs Archive CoreAddons Solid ThreadWeaver GlobalAccel
                                            Config Auth JS Wallet DBusAddons I18n GuiAddons ConfigWidgets
                                            Service ItemViews Notifications IconThemes Completion JobWidgets Sonnet TextWidgets XmlGui Crash
                                            Bookmarks Declarative UnitConversion Parts Kross KIO DNSSD KDE4Support)

find_package(Phonon4Qt5 REQUIRED NO_MODULE)

# find_package(QApt QUIET)
# find_package(DebconfKDE QUIET)
# find_package(QtOAuth REQUIRED)
# find_package(QCA2 REQUIRED) #required by QtOAuth
# find_package(LibAttica QUIET)
# find_package(BODEGA QUIET)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${QAPT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR} ${QCA2_INCLUDE_DIR}
                    ${PHONON_INCLUDES}
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
