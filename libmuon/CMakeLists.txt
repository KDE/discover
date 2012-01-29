set(muon_LIB_SRCS
    ChangesDialog.cpp
    MuonMainWindow.cpp
    MuonStrings.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
    mobile/src/mousecursor.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp)

kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.2.95
    SOVERSION 1
)

target_link_libraries(muonprivate
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QAPT_LIBRARY})

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/mobile/qml/*.qml")

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/libmuon)

