add_subdirectory(libsnapclient)

add_library(snap-backend MODULE
    SnapResource.cpp
    SnapBackend.cpp
    SnapTransaction.cpp
    snapui.qrc
)
target_link_libraries(snap-backend
    Qt::Gui
    Qt::Core
    Qt::Concurrent
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
    Snapd::Core
)

if ("${Snapd_VERSION}" VERSION_GREATER 1.40)
    target_compile_definitions(snap-backend PRIVATE -DSNAP_COMMON_IDS -DSNAP_CHANNELS)
endif()
if ("${Snapd_VERSION}" VERSION_GREATER 1.42)
    target_compile_definitions(snap-backend PRIVATE -DSNAP_PUBLISHER)
endif()
if ("${Snapd_VERSION}" VERSION_GREATER 1.45)
    target_compile_definitions(snap-backend PRIVATE -DSNAP_MEDIA)
endif()
if ("${Snapd_VERSION}" VERSION_GREATER 1.48)
    target_compile_definitions(snap-backend PRIVATE -DSNAP_FIND_COMMON_ID -DSNAP_MARKDOWN)
endif()

install(TARGETS snap-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
install(FILES org.kde.discover.snap.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
