project(muon)

set(CMAKE_MODULE_PATH "${muon_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(KDE_MIN_VERSION "4.7.0")
find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)
find_package(QApt QUIET)
find_package(DebconfKDE QUIET)
find_package(QtOAuth REQUIRED)
find_package(QCA2 REQUIRED) #required by QtOAuth
find_package(QJSON REQUIRED)
find_package(LibAttica QUIET)
find_package(BODEGA QUIET)


include (KDE4Defaults)
include( FindPkgConfig )

if(NOT qjson_LIBRARIES) #hack to compatibilize different qjson finder versions
    set(qjson_LIBRARIES ${QJSON_LIBRARIES})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${QAPT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR} ${QCA2_INCLUDE_DIR}
                    ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_BINARY_DIR}/libmuon)

add_subdirectory(libmuon)
if(QAPT_FOUND)
	add_subdirectory(libmuonapt)
    add_subdirectory(muon)
    add_subdirectory(installer)
    add_subdirectory(updater)
    add_subdirectory(kded)
endif(QAPT_FOUND)
add_subdirectory(discover)
add_subdirectory(exporter)

macro_log_feature(QAPT_FOUND "LibQApt" "Qt wrapper around the libapt-pkg library"
                  "http://www.kde.org" FALSE "" "Required to build the APT backend")
macro_log_feature(LIBATTICA_FOUND "LibAttica" "Qt library that implements the Open Collaboration Services API"
                  "http://www.kde.org" FALSE "" "Required to build the KNewStuff3 backend")
macro_log_feature(BODEGA_FOUND "Bodega" "Library that exposes Bodega resources"
                  "http://www.kde.org" FALSE "" "Required to build the Bodega backend")

macro_display_feature_log()
