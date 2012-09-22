add_subdirectory(icons)

add_definitions(-DQAPT_ENABLED)

set(muon_SRCS
    main.cpp
    MainWindow.cpp
    ManagerWidget.cpp
    ReviewWidget.cpp
    CommitWidget.cpp
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
    DownloadWidget.cpp
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
    config/ManagerSettingsDialog.cpp
    config/GeneralSettingsPage.cpp)

kde4_add_kcfg_files(muon_SRCS GENERATE_MOC config/MuonSettings.kcfgc)
install(FILES config/muon.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_add_executable(muon ${muon_SRCS})

target_link_libraries(muon muonprivate
                           ${KDE4_KIO_LIBS}
                           ${QAPT_LIBRARY}
                           ${DEBCONF_KDE_LIB})

install(TARGETS muon ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS muon.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES muonui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
