add_subdirectory(tests)

set(dummy-backend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
    DummySourcesBackend.cpp
)

add_library(muon-dummy-backend MODULE ${dummy-backend_SRCS})
target_link_libraries(muon-dummy-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::Service muonprivate)

install(TARGETS muon-dummy-backend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-dummy-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-dummy-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)


add_library(kded_muondummyotifier MODULE ${dummynotifier_SRCS})

target_link_libraries(kded_muondummyotifier muonprivate)

install(TARGETS kded_muondummyotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muondummynotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
