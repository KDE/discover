set(muon_updater_SRCS
    main.cpp
    UpdaterWindow.cpp
    UpdaterWidget.cpp
    config/UpdaterSettingsDialog.cpp)

kde4_add_executable(muon-updater ${muon_updater_SRCS})

target_link_libraries(muon-updater libmuon
                                   ${KDE4_KDEUI_LIBS}
                                   ${KDE4_KIO_LIBS}
                                   ${KDE4_SOLID_LIBS}
                                   ${QAPT_LIBRARY}
                                   ${DEBCONF_KDE_LIB})

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muon-updaterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-updater )

