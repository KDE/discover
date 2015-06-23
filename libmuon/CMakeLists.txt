add_definitions(-DTRANSLATION_DOMAIN=\"libmuon\")

add_subdirectory(backends)
add_subdirectory(declarative)
add_subdirectory(notifiers)

set(muon_LIB_SRCS
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
    MuonBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
)

kconfig_add_kcfg_files(muon_LIB_SRCS GENERATE_MOC MuonDataSources.kcfgc)
add_library(MuonCommon ${muon_LIB_SRCS})

target_link_libraries(MuonCommon
LINK_PUBLIC
    Qt5::Core
    Qt5::Widgets
    KF5::I18n
LINK_PRIVATE
    Qt5::Xml
    KF5::Notifications
    KF5::XmlGui
    KF5::ItemViews
    KF5::KIOWidgets
    KF5::IconThemes
)

generate_export_header(MuonCommon EXPORT_FILE_NAME libMuonCommon_export.h)

target_include_directories(MuonCommon PRIVATE ${PHONON_INCLUDES} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS MuonCommon ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES resources/muonabstractnotifier.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
