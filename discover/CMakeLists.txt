add_subdirectory(icons)
add_subdirectory(autotests)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

set(plasma_discover_SRCS
    main.cpp
    DiscoverMainWindow.cpp

    PaginateModel.cpp
    SystemFonts.cpp
    IconColors.cpp
    UnityLauncher.cpp
)

qt5_add_resources(plasma_discover_SRCS resources.qrc)

add_executable(plasma-discover ${plasma_discover_SRCS})
add_executable(Plasma::Discover ALIAS plasma-discover)
set_target_properties(plasma-discover PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover LINK_PUBLIC
                                     KF5::Crash
                                     KF5::DBusAddons
                                     KF5::I18n
                                     KF5::XmlGui
                                     KF5::Declarative
                                     Qt5::Quick
                                     Discover::Common
)

install(TARGETS plasma-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS org.kde.discover.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES featured.json DESTINATION ${DATA_INSTALL_DIR}/plasmadiscover)
install(FILES plasmadiscoverui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/plasmadiscover)
install( FILES org.kde.discover.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
