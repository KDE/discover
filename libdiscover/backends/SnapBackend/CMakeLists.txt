add_subdirectory(tests)
add_subdirectory(libsnapclient)

add_library(snap-backend MODULE SnapResource.cpp SnapBackend.cpp SnapReviewsBackend.cpp SnapTransaction.cpp)
target_link_libraries(snap-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::ConfigCore Discover::Common Snap::Client)

install(TARGETS snap-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES snap-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
install(FILES snap-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

# add_library(SnapNotifier MODULE SnapNotifier.cpp)
# target_link_libraries(SnapNotifier Discover::Notifiers)
# set_target_properties(SnapNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
#
# install(TARGETS SnapNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
