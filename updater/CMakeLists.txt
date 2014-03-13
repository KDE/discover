include_directories(../libmuon)

set(muon_updater_SRCS
    main.cpp
    MainWindow.cpp
    ChangelogWidget.cpp
    ProgressWidget.cpp
    UpdaterWidget.cpp
    KActionMessageWidget.cpp
    UpdateModel/UpdateItem.cpp
    UpdateModel/UpdateModel.cpp
    UpdateModel/UpdateDelegate.cpp
    config/UpdaterSettingsDialog.cpp)

kde4_add_ui_files(muon_updater_SRCS ProgressWidget.ui
                                    UpdaterCentralWidget.ui
                                    UpdaterWidgetNoUpdates.ui
)

kde4_add_executable(muon-updater ${muon_updater_SRCS})

target_link_libraries(muon-updater muonprivate
                                   ${KDE4_KIO_LIBS}
                                   ${KDE4_SOLID_LIBS})

if (ENABLE_QZEITGEIST)
    target_link_libraries(muon-updater ${QZEITGEIST_LIBRARY})
endif ()

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muon-updaterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-updater )
install( PROGRAMS muon-updater.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )

