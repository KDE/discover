add_subdirectory(icons)
add_subdirectory(autotests)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

set(plasma_discover_SRCS
    main.cpp
    MuonDiscoverMainWindow.cpp

    PaginateModel.cpp
    SystemFonts.cpp
    IconColors.cpp
)

qt5_add_resources(plasma_discover_SRCS resources.qrc)

add_executable(plasma-discover ${plasma_discover_SRCS})

target_link_libraries(plasma-discover LINK_PUBLIC
                                     KF5::DBusAddons
                                     KF5::I18n
                                     KF5::XmlGui
                                     Qt5::Quick
                                     Discover::Common
)

install(TARGETS plasma-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS org.kde.discover.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES featured.json DESTINATION ${DATA_INSTALL_DIR}/plasmadiscover)
install(FILES plasmadiscoverui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/plasmadiscover)
