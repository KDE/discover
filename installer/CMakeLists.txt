include_directories(../libmuon ../libmuonapt/)

set(muon_installer_SRCS
    main.cpp
    MainWindow.cpp
    ApplicationLauncher.cpp
    ResourceView/ResourceDelegate.cpp
    ResourceView/ResourceExtender.cpp
    ResourceView/ResourceListView.cpp
    ResourceView/ResourceViewWidget.cpp
    ResourceDetailsView/ResourceDetailsWidget.cpp
    ResourceDetailsView/ResourceDetailsView.cpp
    ResourceDetailsView/AddonsWidget.cpp
    AbstractViewBase.cpp
    AbstractViewContainer.cpp
    AvailableView.cpp
    BreadcrumbWidget/BreadcrumbItem.cpp
    BreadcrumbWidget/BreadcrumbWidget.cpp
    BreadcrumbWidget/BreadcrumbItemButton.cpp
    CategoryView/CategoryDrawer.cpp
    CategoryView/CategoryView.cpp
    CategoryView/CategoryViewWidget.cpp
    ClickableLabel.cpp
    effects/GraphicsOpacityDropShadowEffect.cpp
    ProgressView.cpp
    qml/src/mousecursor.cpp
    ReviewsWidget/ReviewWidget.cpp
    ReviewsWidget/ReviewsWidget.cpp
    ScreenShotOverlay.cpp
    ViewSwitcher.cpp
    LaunchListModel.cpp
)

kconfig_add_kcfg_files(muon_installer_SRCS GENERATE_MOC config/MuonInstallerSettings.kcfgc)

add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer MuonCommon MuonApt
                                     KF5::KIOWidgets
                                     QApt::Main
                                     Qt5::Declarative)

install(TARGETS muon-installer ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/qml/qml/*.qml")
install( PROGRAMS muon-installer.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES muoninstallerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muoninstaller)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/muoninstaller)
install(FILES config/muon-installer.kcfg DESTINATION ${KCFG_INSTALL_DIR})
