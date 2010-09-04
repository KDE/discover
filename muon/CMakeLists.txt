set(muon_SRCS
    main.cpp
    MainWindow.cpp
    ManagerWidget.cpp
    ReviewWidget.cpp
    FilterWidget.cpp)

kde4_add_kcfg_files(muon_SRCS GENERATE_MOC config/MuonSettings.kcfgc)
install(FILES config/muon.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_add_executable(muon ${muon_SRCS})

target_link_libraries(muon ${KDE4_KDEUI_LIBS}
                           ${KDE4_KIO_LIBS}
                           ${KDE4_SOLID_LIBS}
                           ${KDE4_PHONON_LIBS}
                           ${QAPT_LIBRARY}
                           libmuon
                           debconf-kde)

install(TARGETS muon ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES muon.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES muonui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
install( FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/muon )
