add_subdirectory(tests)

set(knsbackend_SRCS
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp
    KNSUpdater.cpp
)

kde4_add_plugin(muon-knsbackend SHARED ${knsbackend_SRCS})
target_link_libraries(muon-knsbackend muonprivate
        ${LIBATTICA_LIBRARIES} ${KDE4_KNEWSTUFF3_LIBS})

install(TARGETS muon-knsbackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-knscomics-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES muon-knsplasmoids-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES knsplasmoids-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)
install(FILES knscomics-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)