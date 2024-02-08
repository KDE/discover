find_package(KF6 REQUIRED Notifications)

ecm_qt_declare_logging_category(libdiscover-backend-packagekit-logging-category_SRCS
    HEADER libdiscover_backend_packagekit_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_PACKAGEKIT_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.packagekit
    DESCRIPTION "libdiscover (backend - PackageKit)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-packagekit-logging-category STATIC
    ${libdiscover-backend-packagekit-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-packagekit-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-packagekit-logging-category
    PRIVATE
        Qt::Core
)

#packagekit-backend
set(packagekit-backend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    PackageKitDependencies.cpp
    AppPackageKitResource.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
    PackageKitMessages.cpp
    PackageKitSourcesBackend.cpp
    LocalFilePKResource.cpp
    PKResolveTransaction.cpp
    pkui.qrc
)

add_library(packagekit-backend MODULE ${packagekit-backend_SRCS})

target_link_libraries(packagekit-backend
    PRIVATE
        Discover::Common
        Qt::Core
        Qt::Concurrent
        PK::packagekitqt6
        KF6::ConfigGui
        KF6::KIOCore
        KF6::KIOGui
        KF6::Archive
        AppStreamQt
        QCoro::Core
        libdiscover-backend-packagekit-logging-category
)

install(TARGETS packagekit-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)

if (TARGET PkgConfig::Markdown)
    target_compile_definitions(packagekit-backend PRIVATE -DWITH_MARKDOWN)
    target_link_libraries(packagekit-backend PRIVATE PkgConfig::Markdown)
endif()

#notifier
set(DiscoverPackageKitNotifier_SRCS
    PackageKitNotifier.cpp
)

add_library(DiscoverPackageKitNotifier MODULE ${DiscoverPackageKitNotifier_SRCS})

target_link_libraries(DiscoverPackageKitNotifier
    PRIVATE
        PK::packagekitqt6
        Discover::Notifiers
        KF6::I18n
        KF6::Notifications
        KF6::ConfigCore
        DiscoverAppStreamIntegration
        libdiscover-backend-packagekit-logging-category
)

set_target_properties(DiscoverPackageKitNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DiscoverPackageKitNotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)

install(FILES packagekit-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

install(FILES org.kde.discover.packagekit.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

option(PACKAGEKIT_AUTOREMOVE "Enable auto-removing unused dependencies when removing packages with PackageKit." "OFF")
if (PACKAGEKIT_AUTOREMOVE)
    add_definitions(-DPACKAGEKIT_AUTOREMOVE)
endif()
