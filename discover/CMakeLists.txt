add_subdirectory(icons)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()    

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

ecm_qt_declare_logging_category(plasma_discover_SRCS HEADER discover_debug.h IDENTIFIER DISCOVER_LOG CATEGORY_NAME org.kde.plasma.discover)

add_executable(plasma-discover ${plasma_discover_SRCS}
    main.cpp
    DiscoverObject.cpp
    DiscoverDeclarativePlugin.cpp

    FeaturedModel.cpp
    PaginateModel.cpp
    UnityLauncher.cpp
    ReadFile.cpp

    resources.qrc
    assets.qrc
)
add_executable(Plasma::Discover ALIAS plasma-discover)
set_target_properties(plasma-discover PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover PUBLIC
                                     KF5::Crash
                                     KF5::DBusAddons
                                     KF5::I18n
                                     KF5::XmlGui
                                     KF5::ItemModels
                                     KF5::KIOWidgets
                                     KF5::QuickAddons
                                     Qt5::Quick
                                     Discover::Common
)

install(TARGETS plasma-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

# if (BUILD_DummyBackend)
#     target_compile_definitions(plasma-discover PRIVATE $<$<CONFIG:Debug>:QT_QML_DEBUG=1>)
# endif()

# Standard desktop file accepts local files as input.
set(DesktopNoDisplay "false")
find_program(DPKG dpkg)
find_program(RPM rpm)
set(DesktopMimeType "")
if(DPKG)
     set(DesktopMimeType "${DesktopMimeType}application/vnd.debian.binary-package;")
endif()
if(RPM)
     set(DesktopMimeType "${DesktopMimeType}application/x-rpm;")
endif()
set(DesktopExec "plasma-discover %F")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

# Support appstream:// URI
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/appstream;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

# support snap:/ URI
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/snap;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.snap.urlhandler.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.snap.urlhandler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

if(EXISTS "/etc/debian_version")
    set(DesktopNoDisplay "true")
    set(DesktopMimeType "x-scheme-handler/apt")
    set(DesktopExec "plasma-discover %U")
    configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.apt.urlhandler.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
endif()

install(FILES plasmadiscoverui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/plasmadiscover)
install( FILES org.kde.discover.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
