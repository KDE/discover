set(muon_installer_SRCS
    main.cpp
    Application.cpp
    ApplicationBackend.cpp
    ApplicationWindow.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationProxyModel.cpp
    ApplicationModel/ApplicationDelegate.cpp
    ApplicationModel/ApplicationExtender.cpp
    ApplicationModel/ApplicationListView.cpp
    ApplicationModel/ApplicationViewWidget.cpp
    ApplicationDetailsView/ApplicationDetailsWidget.cpp
    ApplicationDetailsView/ApplicationDetailsView.cpp
    AbstractViewBase.cpp
    AbstractViewContainer.cpp
    AvailableView.cpp
    BreadcrumbWidget/BreadcrumbItem.cpp
    BreadcrumbWidget/BreadcrumbWidget.cpp
    BreadcrumbWidget/BreadcrumbItemButton.cpp
    CategoryView/Category.cpp
    CategoryView/CategoryDrawer.cpp
    CategoryView/CategoryView.cpp
    CategoryView/CategoryViewWidget.cpp
    ClickableLabel.cpp
    effects/GraphicsOpacityDropShadowEffect.cpp
    ViewSwitcher.cpp
    ScreenShotViewer.cpp)

kde4_add_kcfg_files(muon_installer_SRCS GENERATE_MOC config/MuonInstallerSettings.kcfgc)

kde4_add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer ${KDE4_KDEUI_LIBS}
                                     ${KDE4_KIO_LIBS}
                                     ${KDE4_SOLID_LIBS}
                                     ${QAPT_LIBRARY}
                                     ${KDE4_NEPOMUK_LIBS}
                                     ${DEBCONF_KDE_LIB}
                                     libmuon)

install(TARGETS muon-installer ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

#FIXME
# install( PROGRAMS muon-installer.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES muon-installerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-installer)
install(FILES categories.xml DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
install(FILES config/muon-installer.kcfg DESTINATION ${KCFG_INSTALL_DIR})
