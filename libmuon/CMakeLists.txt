add_subdirectory(backends)
add_subdirectory(declarative)

set(muon_LIB_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
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
    MuonBackendsFactory.cpp
    ScreenshotsModel.cpp
    ApplicationAddonsModel.cpp
)

kde4_add_kcfg_files(muon_LIB_SRCS GENERATE_MOC MuonDataSources.kcfgc)
kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.3.65
    SOVERSION 1
)

target_link_libraries(muonprivate
    ${QT_QTXML_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_PHONON_LIBS}
)

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)

install(FILES
    muonbackendplugin.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR}
)
