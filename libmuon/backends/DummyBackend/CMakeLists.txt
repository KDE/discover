add_subdirectory(tests)

set(dummybackend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
)

kde4_add_plugin(muon-dummybackend SHARED ${dummybackend_SRCS})
target_link_libraries(muon-dummybackend ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBRARY} muonprivate)

install(TARGETS muon-dummybackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-dummy-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES dummy-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)
