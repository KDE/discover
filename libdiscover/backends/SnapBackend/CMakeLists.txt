add_subdirectory(libsnapclient)

add_library(snap-backend MODULE SnapResource.cpp SnapBackend.cpp SnapReviewsBackend.cpp SnapTransaction.cpp)
target_link_libraries(snap-backend Qt5::Core KF5::CoreAddons KF5::ConfigCore Discover::Common Snapd::Core)

install(TARGETS snap-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES snap-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
