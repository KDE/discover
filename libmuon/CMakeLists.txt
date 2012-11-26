add_subdirectory(backends)
add_subdirectory(tests)

set(muon_LIB_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    MuonMainWindow.cpp
    mobile/src/mousecursor.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/AbstractLoginBackend.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
    resources/PackageState.cpp
    resources/AbstractBackendUpdater.cpp
    resources/ResourcesUpdatesModel.cpp
    MuonBackendsFactory.cpp
)

if(QAPT_FOUND)
    set(muon_LIB_SRCS ${muon_LIB_SRCS}
        ChangesDialog.cpp
        MuonStrings.cpp
        QAptActions.cpp
        HistoryView/HistoryProxyModel.cpp
        HistoryView/HistoryView.cpp
    )
    add_definitions(-DQAPT_ENABLED)
endif(QAPT_FOUND)

kde4_add_kcfg_files(muon_LIB_SRCS GENERATE_MOC MuonDataSources.kcfgc)
kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.3.65
    SOVERSION 1
)

target_link_libraries(muonprivate
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QAPT_LIBRARY}
)
if(QAPT_FOUND)
    target_link_libraries(muonprivate ${QAPT_LIBRARY})
endif(QAPT_FOUND)

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/mobile/qml/*.qml")

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES categories.xml DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
