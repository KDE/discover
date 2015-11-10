add_subdirectory(tests)

set(dummy-backend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
    DummySourcesBackend.cpp
)

add_library(dummy-backend MODULE ${dummy-backend_SRCS})
target_link_libraries(dummy-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::Service Discover::Common)

install(TARGETS dummy-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES dummy-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
install(FILES dummy-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

add_library(DummyNotifier MODULE DummyNotifier.cpp)
target_link_libraries(DummyNotifier Discover::Notifiers)

install(TARGETS DummyNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
