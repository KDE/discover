find_package(KF5KIO REQUIRED)
find_package(KF5Archive REQUIRED)

add_library(packagekit-backend MODULE PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
    PackageKitSourcesBackend.cpp
    AppstreamReviews.cpp
)
target_link_libraries(packagekit-backend PRIVATE Discover::Common Qt5::Core PK::packagekitqt5 KF5::ConfigGui KF5::Service KF5::KIOWidgets KF5::Archive AppstreamQt)

install(TARGETS packagekit-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES packagekit-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)

#notifier
add_library(DiscoverPackageKitNotifier MODULE PackageKitNotifier.cpp)
target_link_libraries(DiscoverPackageKitNotifier PRIVATE PK::packagekitqt5 Discover::Notifiers)
set_target_properties(DiscoverPackageKitNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DiscoverPackageKitNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)

install(FILES packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
add_subdirectory(categoryimages)
