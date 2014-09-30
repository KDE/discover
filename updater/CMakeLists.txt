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

qt5_wrap_ui(muon_updater_SRCS ProgressWidget.ui
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
                                   muonprivate)

if (ENABLE_QZEITGEIST)
    target_link_libraries(muon-updater ${QZEITGEIST_LIBRARY})
endif ()

install(TARGETS muon-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES muon-updaterui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/muon-updater )
install( PROGRAMS muon-updater.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )

