include_directories(../libdiscover)

set(plasmadiscover_updater_SRCS
    main.cpp
    MainWindow.cpp
    ChangelogWidget.cpp
    ProgressWidget.cpp
    UpdaterWidget.cpp
    KActionMessageWidget.cpp
    UpdateDelegate.cpp)

ki18n_wrap_ui(plasmadiscover_updater_SRCS ProgressWidget.ui
                                    UpdaterCentralWidget.ui
                                    UpdaterWidgetNoUpdates.ui
)

add_executable(plasma-discover-updater ${plasmadiscover_updater_SRCS})

target_link_libraries(plasma-discover-updater KF5::CoreAddons
                                   KF5::KIOWidgets
                                   KF5::I18n
                                   KF5::DBusAddons
                                   KF5::XmlGui
                                   KF5::IconThemes
                                   Discover::Common)

if (ENABLE_QZEITGEIST)
    target_link_libraries(plasma-discover-updater ${QZEITGEIST_LIBRARY})
endif ()

install(TARGETS plasma-discover-updater ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES plasmadiscoverupdaterui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/plasmadiscoverupdater )
install( PROGRAMS plasma-discover-updater.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )

