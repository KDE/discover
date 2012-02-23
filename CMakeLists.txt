project(muon)

set(KDE_MIN_VERSION "4.5.0")
find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)
find_package(QApt REQUIRED)
find_package(DebconfKDE REQUIRED)
find_package(QZeitgeist)
find_package(QJSON REQUIRED)

include (KDE4Defaults)

include( FindPkgConfig )

OPTION (ENABLE_QZEITGEIST "QZeitgeist support for App usage statistics" ON)

# Used to determine whether system or backported KMessageWidget is used
macro_ensure_version("4.6.60" ${KDE_VERSION} HAVEKDE4_6_60)

if (QZEITGEIST_INCLUDE_DIR)
    MESSAGE(STATUS "Found QZEITGEIST:")
    if (ENABLE_QZEITGEIST)
        set(HAVE_QZEITGEIST TRUE)
    endif (ENABLE_QZEITGEIST)
else (QZEITGEIST_INCLUDE_DIR)
    set(ENABLE_QZEITGEIST FALSE)
endif (QZEITGEIST_INCLUDE_DIR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

macro_log_feature(qjson_FOUND "QJson" "Qt-based library that maps JSON data to QVariant objects"
                  "http://qjson.sourceforge.net/" FALSE "" "Used for review fetching")
macro_log_feature(QAPT_FOUND "QApt" "High-level Qt wrapper/Apt implementation around the Apt-pkg library"
                  "http://kubuntu.org" FALSE "" "Muon's package management backend")
macro_log_feature(HAVE_QZEITGEIST "LibQZeitgeist" "Qt wrapper around the Zeitgeist DBus event logging service"
                  "http://launchpad.net/zeitgeist/" FALSE "" "Used for application usage counting")

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${QAPT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/HaveQZeitgeist.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/HaveQZeitgeist.h)

add_subdirectory(libmuon)
add_subdirectory(muon)
add_subdirectory(installer)
add_subdirectory(muon-mobile)
add_subdirectory(updater)
add_subdirectory(kded)

macro_display_feature_log()
