find_package(KF6Auth CONFIG REQUIRED)  # Probably should be moved to top CMakeLists
find_package(KF6JobWidgets CONFIG REQUIRED)

set(alpineapkbackend_SRCS
    AlpineApkAuthActionFactory.h
    AlpineApkAuthActionFactory.cpp
    AlpineApkBackend.cpp
    AlpineApkBackend.h
    AlpineApkResource.cpp
    AlpineApkResource.h
    AlpineApkReviewsBackend.cpp
    AlpineApkReviewsBackend.h
    AlpineApkSourcesBackend.cpp
    AlpineApkSourcesBackend.h
    AlpineApkUpdater.cpp
    AlpineApkUpdater.h
    AlpineApkTransaction.cpp
    AlpineApkTransaction.h
    AppstreamDataDownloader.h
    AppstreamDataDownloader.cpp
)

ecm_qt_declare_logging_category(
    alpineapkbackend_SRCS  # sources_var
    HEADER alpineapk_backend_logging.h
    IDENTIFIER LOG_ALPINEAPK
    CATEGORY_NAME org.kde.plasma.discover.alpineapk
    DEFAULT_SEVERITY Debug
)

add_library(
    alpineapk-backend
    MODULE
        ${alpineapkbackend_SRCS}
)

target_link_libraries(
    alpineapk-backend
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Concurrent
        KF6::AuthCore
        KF6::ConfigCore
        KF6::CoreAddons
        KF6::JobWidgets
        KF6::KIOCore
        Discover::Common
        ApkQt::ApkQt
        AppStreamQt
)

# KAuth helper exe
add_executable(alpineapk_kauth_helper
    AlpineApkAuthHelper.cpp
    AlpineApkAuthHelper.h
    org.kde.discover.alpineapkbackend.actions
)
set_source_files_properties(
    org.kde.discover.alpineapkbackend.actions
    PROPERTIES HEADER_FILE_ONLY ON
)
target_link_libraries(alpineapk_kauth_helper
    Qt::Core
    KF6::AuthCore
    ApkQt::ApkQt
)

kauth_install_actions(org.kde.discover.alpineapkbackend org.kde.discover.alpineapkbackend.actions)
kauth_install_helper_files(alpineapk_kauth_helper org.kde.discover.alpineapkbackend root)

install(
    TARGETS alpineapk-backend
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover
)

install(
    TARGETS alpineapk_kauth_helper
    DESTINATION ${KAUTH_HELPER_INSTALL_DIR}
)

# add_library(AlpineApkNotifier MODULE AlpineApkNotifier.cpp)

# target_link_libraries(AlpineApkNotifier Discover::Notifiers)

# set_target_properties(AlpineApkNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

# install(TARGETS AlpineApkNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
