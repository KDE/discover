add_subdirectory(libsnapclient)

add_library(snap-backend MODULE SnapResource.cpp SnapBackend.cpp SnapReviewsBackend.cpp SnapTransaction.cpp snapui.qrc)
target_link_libraries(snap-backend Qt5::Core KF5::CoreAddons KF5::ConfigCore Discover::Common Snapd::Core)

if ("${Snapd_VERSION}" VERSION_GREATER 1.40)
    target_compile_definitions(snap-backend PRIVATE -DSNAP_COMMON_IDS)
endif()

install(TARGETS snap-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES snap-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
install( FILES org.kde.discover.snap.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
