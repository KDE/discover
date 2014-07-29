add_subdirectory(tests)

set(dummybackend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
)

add_library(muon-dummybackend MODULE ${dummybackend_SRCS})
target_link_libraries(muon-dummybackend Qt5::Core Qt5::Widgets KF5::CoreAddons muonprivate)

install(TARGETS muon-dummybackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-dummy-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES muon-dummy-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)


add_library(kded_muondummyotifier MODULE ${dummynotifier_SRCS})

target_link_libraries(kded_muondummyotifier ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} muonprivate)

install(TARGETS kded_muondummyotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muondummynotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
