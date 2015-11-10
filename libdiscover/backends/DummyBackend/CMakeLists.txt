add_subdirectory(tests)

set(dummy-backend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
    DummySourcesBackend.cpp
)

add_library(muon-dummy-backend MODULE ${dummy-backend_SRCS})
target_link_libraries(muon-dummy-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::Service DiscoverCommon)

install(TARGETS muon-dummy-backend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-dummy-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-dummy-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_library(MuonDummyNotifier MODULE DummyNotifier.cpp)
target_link_libraries(MuonDummyNotifier DiscoverNotifiers)

install(TARGETS MuonDummyNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/muon-notifier)
