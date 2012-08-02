project(muon)

set(CMAKE_MODULE_PATH "${muon_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(KDE_MIN_VERSION "4.7.0")
find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)
find_package(QApt REQUIRED)
find_package(DebconfKDE REQUIRED)
find_package(QtOAuth REQUIRED)
find_package(QCA2 REQUIRED) #required by QtOAuth
find_package(QJSON REQUIRED)

include (KDE4Defaults)

include( FindPkgConfig )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES} ${QAPT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QCA2_INCLUDE_DIR})

add_subdirectory(libmuon)
add_subdirectory(muon)
add_subdirectory(installer)
add_subdirectory(discover)
add_subdirectory(updater)
add_subdirectory(kded)

macro_display_feature_log()
