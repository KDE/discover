add_subdirectory(backends)
add_subdirectory(declarative)

set(muon_LIB_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    Category/CategoriesReader.cpp
    MuonMainWindow.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/AbstractLoginBackend.cpp
    ReviewsBackend/ReviewsModel.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp
    Transaction/AddonList.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    Transaction/TransactionModel.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
    resources/PackageState.cpp
    resources/AbstractBackendUpdater.cpp
    resources/ResourcesUpdatesModel.cpp
    resources/StandardBackendUpdater.cpp
    resources/AbstractKDEDModule.cpp
    MuonBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
)

kconfig_add_kcfg_files(muon_LIB_SRCS GENERATE_MOC MuonDataSources.kcfgc)
add_library(muonprivate ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 2.1.3
    SOVERSION 2
)

target_link_libraries(muonprivate
LINK_PUBLIC
    Qt5::Core
    Qt5::Widgets
    KF5::Service
    KF5::DBusAddons # for the notifier
    KF5::KDELibs4Support
LINK_PRIVATE
    Qt5::Xml
    KF5::XmlGui
    KF5::ItemViews
    Phonon::phonon4qt5
)

target_include_directories(muonprivate PRIVATE ${PHONON_INCLUDES})

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES resources/muonabstractnotifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muonabstractnotifier)

install(FILES
    muonbackendplugin.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR}
)
