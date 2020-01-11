# add_subdirectory(tests) # no tests yet

set(alpineapkbackend_SRCS
    AlpineApkBackend.cpp
    AlpineApkBackend.h
    AlpineApkResource.cpp
    AlpineApkResource.h
    AlpineApkSourcesBackend.cpp
    AlpineApkSourcesBackend.h
    AlpineApkTransaction.cpp
    AlpineApkTransaction.h
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
        Qt5::Core
        Qt5::Widgets
        KF5::CoreAddons
        KF5::ConfigCore
        Discover::Common
        apk-qt
)

install(
    TARGETS alpineapk-backend
    DESTINATION ${PLUGIN_INSTALL_DIR}/discover
)

# install(FILES alpineapk-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

# add_library(AlpineApkNotifier MODULE AlpineApkNotifier.cpp)

# target_link_libraries(AlpineApkNotifier Discover::Notifiers)

# set_target_properties(AlpineApkNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

# install(TARGETS AlpineApkNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
