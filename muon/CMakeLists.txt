find_package(KF5KDELibs4Support)

add_subdirectory(icons)

set(muon_SRCS
    main.cpp
    MainWindow.cpp
    ManagerWidget.cpp
    ReviewWidget.cpp
    DetailsTabs/DetailsTab.cpp
    DetailsTabs/MainTab.cpp
    DetailsTabs/ChangelogTab.cpp
    DetailsTabs/DependsTab.cpp
    DetailsTabs/InstalledFilesTab.cpp
    DetailsTabs/TechnicalDetailsTab.cpp
    DetailsTabs/VersionTab.cpp
    DetailsWidget.cpp
    DownloadModel/DownloadModel.cpp
    DownloadModel/DownloadDelegate.cpp
    FilterWidget/ArchitectureFilter.cpp
    FilterWidget/CategoryFilter.cpp
    FilterWidget/FilterModel.cpp
    FilterWidget/FilterWidget.cpp
    FilterWidget/OriginFilter.cpp
    FilterWidget/StatusFilter.cpp
    PackageModel/PackageModel.cpp
    PackageModel/PackageProxyModel.cpp
    PackageModel/PackageView.cpp
    PackageModel/PackageDelegate.cpp
    PackageModel/PackageWidget.cpp
    StatusWidget.cpp
    TransactionWidget.cpp
    config/ManagerSettingsDialog.cpp
    config/GeneralSettingsPage.cpp)

kconfig_add_kcfg_files(muon_SRCS GENERATE_MOC config/MuonSettings.kcfgc)
install(FILES config/muon.kcfg DESTINATION ${KCFG_INSTALL_DIR})

add_executable(muon ${muon_SRCS})

target_link_libraries(muon muonprivate MuonApt
                           DebconfKDE::Main
                           KF5::KIOWidgets
                           KF5::KDELibs4Support
                           Qt5::Concurrent
                           QApt::Main)

install(TARGETS muon ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS muon.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES muonui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
