project(muon)

find_package(KDE4 REQUIRED)
find_package(QApt REQUIRED)
find_package(DebconfKDE REQUIRED)
include (KDE4Defaults)

include( FindPkgConfig )

macro_log_feature(QAPT_FOUND "QApt" "High-level Qt wrapper/Apt implementation around the Apt-pkg library"
                  "http://kubuntu.org" FALSE "" "Muon's package management backend")

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

add_subdirectory( src )

macro_display_feature_log()
