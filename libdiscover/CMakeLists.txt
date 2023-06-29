add_definitions(-DTRANSLATION_DOMAIN=\"libdiscover\")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

add_subdirectory(backends)
add_subdirectory(notifiers)
add_subdirectory(tests)

set(discovercommon_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    Category/CategoriesReader.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/ReviewsModel.cpp
    Transaction/AddonList.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    Transaction/TransactionModel.cpp
    UpdateModel/UpdateItem.cpp
    UpdateModel/UpdateModel.cpp
    resources/DiscoverAction.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/PackageState.cpp
    resources/ResourcesUpdatesModel.cpp
    resources/StandardBackendUpdater.cpp
    resources/SourcesModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
    resources/AbstractBackendUpdater.cpp
    resources/AbstractSourcesBackend.cpp
    resources/StoredResultsStream.cpp
    DiscoverBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
    CachedNetworkAccessManager.cpp
)

ecm_qt_declare_logging_category(discovercommon_SRCS HEADER libdiscover_debug.h IDENTIFIER LIBDISCOVER_LOG CATEGORY_NAME org.kde.plasma.libdiscover DESCRIPTION "libdiscover" EXPORT DISCOVER)


add_library(DiscoverCommon ${discovercommon_SRCS})
if(TARGET AppStreamQt)
    target_sources(DiscoverCommon PRIVATE
        appstream/OdrsReviewsBackend.cpp
        appstream/AppStreamIntegration.cpp
        appstream/AppStreamUtils.cpp
    )
    target_link_libraries(DiscoverCommon PRIVATE Qt::Concurrent AppStreamQt)
endif()

target_link_libraries(DiscoverCommon
PUBLIC
    Qt::Core
    Qt::Qml
    Qt::Gui
    KF6::I18n
PRIVATE
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::KIOCore
)
add_library(Discover::Common ALIAS DiscoverCommon)

generate_export_header(DiscoverCommon)

target_include_directories(DiscoverCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(DiscoverCommon URI org.kde.discover VERSION 2.0 NO_PLUGIN)

target_include_directories(DiscoverCommon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_SOURCE_DIR}/ReviewsBackend ${CMAKE_CURRENT_SOURCE_DIR}/Transaction ${CMAKE_CURRENT_SOURCE_DIR}/Category ${CMAKE_CURRENT_SOURCE_DIR}/UpdateModel)

install(TARGETS DiscoverCommon DESTINATION ${KDE_INSTALL_LIBDIR}/plasma-discover)
install(FILES resources/discoverabstractnotifier.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
