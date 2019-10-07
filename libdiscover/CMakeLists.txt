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
    ActionsModel.cpp
    DiscoverBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
    CachedNetworkAccessManager.cpp
)

ecm_qt_declare_logging_category(discovercommon_SRCS HEADER libdiscover_debug.h IDENTIFIER LIBDISCOVER_LOG CATEGORY_NAME org.kde.plasma.libdiscover)


add_library(DiscoverCommon ${discovercommon_SRCS})
if(AppStreamQt_FOUND)
    target_sources(DiscoverCommon PRIVATE
        appstream/OdrsReviewsBackend.cpp
        appstream/AppStreamIntegration.cpp
        appstream/AppStreamUtils.cpp
    )
    target_link_libraries(DiscoverCommon PRIVATE AppStreamQt)

    if(AppStreamQt_VERSION VERSION_GREATER 0.12.4)
        target_compile_definitions(DiscoverCommon PRIVATE -DAPPSTREAM_HAS_SPDX=1)
    else()
        target_compile_definitions(DiscoverCommon PRIVATE -DAPPSTREAM_HAS_SPDX=0)
    endif()
    if(AppStreamQt_VERSION VERSION_GREATER 0.12.7)
        target_compile_definitions(DiscoverCommon PRIVATE -DAPPSTREAM_HAS_SPDX_LICENSEURL=1)
    else()
        target_compile_definitions(DiscoverCommon PRIVATE -DAPPSTREAM_HAS_SPDX_LICENSEURL=0)
    endif()
endif()

target_link_libraries(DiscoverCommon
LINK_PUBLIC
    Qt5::Core
    Qt5::Qml
    Qt5::Widgets
    KF5::I18n
    KF5::ItemModels
LINK_PRIVATE
    Qt5::Xml
    KF5::CoreAddons
    KF5::KIOWidgets # KIO/AccessManager
)
add_library(Discover::Common ALIAS DiscoverCommon)

generate_export_header(DiscoverCommon)

target_include_directories(DiscoverCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS DiscoverCommon DESTINATION ${CMAKE_INSTALL_LIBDIR}/plasma-discover)
install(FILES resources/discoverabstractnotifier.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
