include_directories(${LIBATTICA_INCLUDE_DIR})

add_subdirectory(tests)

add_library(kns-backend MODULE KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp)

target_link_libraries(kns-backend Discover::Common
        KF5::ConfigCore KF5::Attica KF5::NewStuff KF5::WidgetsAddons)

install(TARGETS kns-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES knscomics-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
install(FILES knsplasmoids-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)

install(FILES knsplasmoids-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
install(FILES knscomics-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
