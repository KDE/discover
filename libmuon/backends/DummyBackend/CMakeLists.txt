add_subdirectory(tests)

set(dummybackend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
)

set(dummynotifier_SRCS
    DummyNotifier.cpp
)

kde4_add_plugin(muon-dummybackend SHARED ${dummybackend_SRCS})
target_link_libraries(muon-dummybackend ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBRARY} muonprivate)

install(TARGETS muon-dummybackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-dummy-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES dummy-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)


kde4_add_plugin(kded_muondummyotifier ${dummynotifier_SRCS})

target_link_libraries(kded_muondummyotifier ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} muonprivate)

install(TARGETS kded_muondummyotifier DESTINATION ${PLUGIN_INSTALL_DIR})

#install(FILES muon-dummy-notifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
install(FILES muondummynotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
#install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
