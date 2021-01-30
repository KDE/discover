find_package(KF5 REQUIRED Notifications)

add_subdirectory(runservice)

#packagekit-backend
set (packagekit-backend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
    PackageKitSourcesBackend.cpp
    LocalFilePKResource.cpp
    PKResolveTransaction.cpp
    pkui.qrc
    )
ecm_qt_declare_logging_category(packagekit-backend_SRCS HEADER libdiscover_backend_debug.h IDENTIFIER LIBDISCOVER_BACKEND_LOG CATEGORY_NAME org.kde.plasma.libdiscover.backend DESCRIPTION "libdiscover backend" EXPORT DISCOVER)

add_library(packagekit-backend MODULE ${packagekit-backend_SRCS})

target_link_libraries(packagekit-backend PRIVATE Discover::Common Qt::Core PK::packagekitqt5 KF5::ConfigGui KF5::KIOCore KF5::Archive AppStreamQt)
install(TARGETS packagekit-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)

if(TARGET PkgConfig::Markdown)
    target_compile_definitions(packagekit-backend PRIVATE -DWITH_MARKDOWN)
    target_link_libraries(packagekit-backend PRIVATE PkgConfig::Markdown)
endif()

#notifier
set (DiscoverPackageKitNotifier_SRCS
    PackageKitNotifier.cpp
)
ecm_qt_declare_logging_category(DiscoverPackageKitNotifier_SRCS HEADER libdiscover_backend_debug.h IDENTIFIER LIBDISCOVER_BACKEND_LOG CATEGORY_NAME org.kde.plasma.libdiscover.backend)

add_library(DiscoverPackageKitNotifier MODULE ${DiscoverPackageKitNotifier_SRCS})

target_link_libraries(DiscoverPackageKitNotifier PRIVATE PK::packagekitqt5 Discover::Notifiers KF5::I18n KF5::Notifications KF5::ConfigCore)
set_target_properties(DiscoverPackageKitNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DiscoverPackageKitNotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)

install(FILES packagekit-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
add_subdirectory(categoryimages)

install( FILES org.kde.discover.packagekit.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
