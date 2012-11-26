include_directories(../libmuon)

add_definitions(-DQAPT_ENABLED)

set(muon_updater_SRCS
    main.cpp
    MainWindow.cpp
    ChangelogWidget.cpp
    ProgressWidget.cpp
    UpdaterWidget.cpp
    UpdateModel/UpdateItem.cpp
    UpdateModel/UpdateModel.cpp
    UpdateModel/UpdateDelegate.cpp
    config/UpdaterSettingsDialog.cpp)

kde4_add_executable(muon-updater ${muon_updater_SRCS})

target_link_libraries(muon-updater muonprivate muon-appsbackend
                                   ${KDE4_KIO_LIBS}
                                   ${KDE4_SOLID_LIBS}
                                   ${QAPT_LIBRARY})

if (ENABLE_QZEITGEIST)
    target_link_libraries(muon-updater ${QZEITGEIST_LIBRARY})
endif (ENABLE_QZEITGEIST)

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muon-updaterui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-updater )
install( PROGRAMS muon-updater.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )

