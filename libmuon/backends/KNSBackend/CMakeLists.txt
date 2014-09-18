include_directories(${LIBATTICA_INCLUDE_DIR})

add_subdirectory(tests)

set(knsbackend_SRCS
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp
)

add_library(muon-knsbackend MODULE ${knsbackend_SRCS})
target_link_libraries(muon-knsbackend muonprivate
        KF5::ConfigCore KF5::Attica KF5::NewStuff)

install(TARGETS muon-knsbackend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-knscomics-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-knsplasmoids-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)

install(FILES muon-knsplasmoids-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)
install(FILES muon-knscomics-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)
