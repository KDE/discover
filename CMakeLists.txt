project(muon)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)

include( FindPkgConfig )

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

pkg_check_modules( EPT REQUIRED "libept >= 0.5.25" )
#FIXME: Fix QApt's cmake file, less ugly that way
pkg_check_modules( LIBQAPT REQUIRED "libqapt >= 0.1" )

add_subdirectory( src )
