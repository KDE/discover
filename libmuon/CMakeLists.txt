set(muon_LIB_SRCS
    MuonMainWindow.cpp
    MuonStrings.cpp
    CommitWidget.cpp
    DownloadWidget.cpp
    StatusWidget.cpp
    DetailsWidget.cpp
    DownloadModel/DownloadModel.cpp
    DownloadModel/DownloadDelegate.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
    PackageModel/PackageModel.cpp
    PackageModel/PackageProxyModel.cpp
    PackageModel/PackageView.cpp
    PackageModel/PackageDelegate.cpp
    PackageModel/PackageWidget.cpp
    DetailsTabs/MainTab.cpp
    DetailsTabs/ChangelogTab.cpp
    DetailsTabs/DependsTab.cpp
    DetailsTabs/InstalledFilesTab.cpp
    DetailsTabs/TechnicalDetailsTab.cpp
    DetailsTabs/VersionTab.cpp
    mobile/src/mousecursor.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp)

kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.2.50
    SOVERSION 1
)

target_link_libraries(muonprivate
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${KDE4_THREADWEAVER_LIBRARIES}
    ${QAPT_LIBRARY}
    ${DEBCONF_KDE_LIB})

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/mobile/qml/*.qml")

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/libmuon)

