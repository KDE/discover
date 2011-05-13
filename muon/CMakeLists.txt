add_subdirectory(icons)

set(muon_SRCS
    main.cpp
    MainWindow.cpp
    ManagerWidget.cpp
    ReviewWidget.cpp
    FilterWidget.cpp
    config/ManagerSettingsDialog.cpp
    config/GeneralSettingsPage.cpp)

kde4_add_kcfg_files(muon_SRCS GENERATE_MOC config/MuonSettings.kcfgc)
install(FILES config/muon.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_add_executable(muon ${muon_SRCS})

target_link_libraries(muon muonprivate
                           ${KDE4_KIO_LIBS}
                           ${QAPT_LIBRARY})

install(TARGETS muon ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS muon.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES muonui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
