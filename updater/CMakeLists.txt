include_directories(../libmuon)

set(muon_updater_SRCS
    main.cpp
    MainWindow.cpp
    ChangelogWidget.cpp
    ProgressWidget.cpp
    UpdaterWidget.cpp
    KActionMessageWidget.cpp
    UpdateDelegate.cpp)

ki18n_wrap_ui(muon_updater_SRCS ProgressWidget.ui
                                    UpdaterCentralWidget.ui
                                    UpdaterWidgetNoUpdates.ui
)

add_executable(muon-updater ${muon_updater_SRCS})

target_link_libraries(muon-updater KF5::CoreAddons
                                   KF5::KIOWidgets
                                   KF5::Solid
                                   KF5::I18n
                                   KF5::DBusAddons
                                   KF5::XmlGui
                                   KF5::IconThemes
                                   MuonCommon)

if (ENABLE_QZEITGEIST)
    target_link_libraries(muon-updater ${QZEITGEIST_LIBRARY})
endif ()

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muonupdaterui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/muonupdater )
install( PROGRAMS muon-updater.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )

