set(muon_SRCS
    main.cpp
    MainWindow.cpp
    MuonStrings.cpp
    ManagerWidget.cpp
    ReviewWidget.cpp
    DownloadWidget.cpp
    CommitWidget.cpp
    FilterWidget.cpp
    DetailsWidget.cpp
    StatusWidget.cpp
    PackageModel/PackageModel.cpp
    PackageModel/PackageProxyModel.cpp
    PackageModel/PackageView.cpp
    PackageModel/PackageDelegate.cpp
    DetailsTabs/MainTab.cpp
    DetailsTabs/ChangelogTab.cpp
    DetailsTabs/DependsTab.cpp
    DetailsTabs/InstalledFilesTab.cpp
    DetailsTabs/TechnicalDetailsTab.cpp
 )

kde4_add_ui_files(muon_SRCS
                  DetailsTabs/MainTab.ui)

kde4_add_kcfg_files(muon_SRCS GENERATE_MOC config/MuonSettings.kcfgc)
install(FILES config/muon.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_add_executable(muon ${muon_SRCS})

target_link_libraries(muon ${KDE4_KDEUI_LIBS}
                           ${KDE4_KIO_LIBS}
                           ${KDE4_SOLID_LIBS}
                           ${QAPT_LIBRARY}
                           debconf-kde)

install(TARGETS muon ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES muon.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES muonui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
