add_definitions(-DTRANSLATION_DOMAIN=\"libdiscover\")

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
    CachedNetworkAccessManager.cpp
    MessageActionsModel
    DiscoverBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
)

if(AppStreamQt_FOUND)
    set(discovercommon_SRCS ${discovercommon_SRCS}
        appstream/OdrsReviewsBackend.cpp
        appstream/AppStreamIntegration.cpp
    )
endif()

add_library(DiscoverCommon ${discovercommon_SRCS})

target_link_libraries(DiscoverCommon
LINK_PUBLIC
    Qt5::Core
    Qt5::Qml
    Qt5::Widgets
    KF5::I18n
LINK_PRIVATE
    Qt5::Xml
    Qt5::DBus
    KF5::KIOCore
    KF5::CoreAddons
)
add_library(Discover::Common ALIAS DiscoverCommon)

generate_export_header(DiscoverCommon)

target_include_directories(DiscoverCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS DiscoverCommon DESTINATION ${CMAKE_INSTALL_LIBDIR}/plasma-discover)
install(FILES resources/discoverabstractnotifier.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
