set(muon_LIB_SRCS
    Application.cpp
    ApplicationBackend.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationProxyModel.cpp
    ChangesDialog.cpp
    MuonMainWindow.cpp
    MuonStrings.cpp
    Category/Category.cpp
    Category/CategoryModel.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
    mobile/src/mousecursor.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/ReviewsBackend.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp)

kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.2.95
    SOVERSION 1
)

if(NOT qjson_LIBRARIES) #hack to compatibilize different qjson finder versions
    set(qjson_LIBRARIES ${QJSON_LIBRARIES})
endif()

target_link_libraries(muonprivate
    ${DEBCONF_KDE_LIB}
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QAPT_LIBRARY}
    ${qjson_LIBRARIES})

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/mobile/qml/*.qml")

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/libmuon)

