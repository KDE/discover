add_subdirectory(icons)
add_subdirectory(autotests)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

qt5_add_resources(plasma_discover_SRCS resources.qrc)
qt5_add_resources(plasma_discover_SRCS assets.qrc)

add_executable(plasma-discover ${plasma_discover_SRCS}
    main.cpp
    DiscoverObject.cpp
    DiscoverDeclarativePlugin.cpp

    FeaturedModel.cpp
    PaginateModel.cpp
    UnityLauncher.cpp
)
add_executable(Plasma::Discover ALIAS plasma-discover)
set_target_properties(plasma-discover PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover LINK_PUBLIC
                                     KF5::Crash
                                     KF5::DBusAddons
                                     KF5::I18n
                                     KF5::XmlGui
                                     KF5::Declarative
                                     KF5::ItemModels
                                     KF5::KIOCore
                                     Qt5::Quick
                                     Discover::Common
)

install(TARGETS plasma-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

# if (BUILD_DummyBackend)
#     target_compile_definitions(plasma-discover PRIVATE $<$<CONFIG:Debug>:QT_QML_DEBUG=1>)
# endif()

# Standard desktop file accepts local files as input.
set(DesktopNoDisplay "false")
set(DesktopMimeType "application/vnd.debian.binary-package;application/x-rpm;")
set(DesktopExec "plasma-discover %F")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

# URL handler accepts URLs as input but is only used to handle appstream://
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/appstream;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

if(EXISTS "/etc/debian_version")
    set(DesktopNoDisplay "true")
    set(DesktopMimeType "x-scheme-handler/apt")
    set(DesktopExec "plasma-discover %U")
    configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
endif()

install(FILES plasmadiscoverui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/plasmadiscover)
install( FILES org.kde.discover.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
