set(muon_updater_SRCS
    main.cpp
    MainWindow.cpp
    ProgressWidget.cpp
    UpdaterWidget.cpp
    UpdateModel/UpdateItem.cpp
    UpdateModel/UpdateModel.cpp
    UpdateModel/UpdateDelegate.cpp
    ../installer/Application.cpp
    config/UpdaterSettingsDialog.cpp)

kde4_add_executable(muon-updater ${muon_updater_SRCS})

target_link_libraries(muon-updater muonprivate
                                   ${KDE4_KIO_LIBS}
                                   ${QAPT_LIBRARY})

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muon-updaterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-updater )

