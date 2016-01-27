add_definitions(-DTRANSLATION_DOMAIN=\"libdiscover\")

add_subdirectory(backends)
add_subdirectory(declarative)
add_subdirectory(notifiers)

set(discovercommon_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    Category/CategoriesReader.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/AbstractLoginBackend.cpp
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
    resources/UIHelper.cpp
    MessageActionsModel
    DiscoverBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
)

kconfig_add_kcfg_files(discovercommon_SRCS GENERATE_MOC MuonDataSources.kcfgc)
add_library(DiscoverCommon ${discovercommon_SRCS})

target_link_libraries(DiscoverCommon
LINK_PUBLIC
    Qt5::Core
    Qt5::Widgets
    KF5::I18n
LINK_PRIVATE
    Qt5::Xml
    KF5::Notifications
    KF5::XmlGui
    KF5::KIOWidgets
    KF5::IconThemes
)
add_library(Discover::Common ALIAS DiscoverCommon)

generate_export_header(DiscoverCommon)

target_include_directories(DiscoverCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS DiscoverCommon ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES resources/discoverabstractnotifier.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
