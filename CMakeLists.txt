project(muon)

find_package(KDE4 REQUIRED)
find_package(QApt REQUIRED)
include (KDE4Defaults)

include( FindPkgConfig )

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

add_subdirectory( src )
