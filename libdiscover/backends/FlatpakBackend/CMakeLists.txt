# add_subdirectory(tests)

include_directories(${FLATPAK_INCLUDE_DIRS} ${APPSTREAM_GLIB_INCLUDE_DIRS})

set(flatpak-backend_SRCS
    FlatpakResource.cpp
    FlatpakBackend.cpp
    FlatpakReviewsBackend.cpp
    FlatpakTransaction.cpp
    FlatpakTransactionJob.cpp
    FlatpakSourcesBackend.cpp
)

add_library(flatpak-backend MODULE ${flatpak-backend_SRCS})
target_link_libraries(flatpak-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::ConfigCore Discover::Common AppStreamQt ${APPSTREAM_GLIB_LIBRARIES} ${FLATPAK_LIBRARIES})

install(TARGETS flatpak-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES flatpak-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

add_library(FlatpakNotifier MODULE FlatpakNotifier.cpp)
target_link_libraries(FlatpakNotifier Discover::Notifiers)
set_target_properties(FlatpakNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS FlatpakNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
