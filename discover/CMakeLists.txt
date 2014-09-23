add_subdirectory(icons)
add_subdirectory(muon-contenttheme)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

set(muon_discover_SRCS
    main.cpp
    MuonDiscoverMainWindow.cpp

    DiscoverAction.cpp
    PaginateModel.cpp
    SystemFonts.cpp
)

qt5_add_resources(muon_discover_SRCS resources.qrc)

add_executable(muon-discover ${muon_discover_SRCS})

target_link_libraries(muon-discover LINK_PUBLIC KF5::KIOWidgets
                                     KF5::Declarative
                                     KF5::DBusAddons
                                     KF5::I18n
                                     KF5::ConfigWidgets
                                     KF5::XmlGui
                                     Qt5::QuickWidgets
                                     muonprivate
                                     ${QAPT_LIBRARY}
)

install(TARGETS muon-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS muon-discover.desktop muon-discover-category.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES featured.json DESTINATION ${DATA_INSTALL_DIR}/muondiscover)
install(FILES muondiscoverui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/muondiscover)
