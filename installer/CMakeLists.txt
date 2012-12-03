include_directories(../libmuon)

add_definitions(-DQAPT_ENABLED)

if (LIBATTICA_FOUND)
    add_definitions(-DATTICA_ENABLED)
endif (LIBATTICA_FOUND)

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
    ViewSwitcher.cpp)

kde4_add_kcfg_files(muon_installer_SRCS GENERATE_MOC config/MuonInstallerSettings.kcfgc)

kde4_add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer muonprivate
                                     ${KDE4_KIO_LIBS}
                                     ${QAPT_LIBRARY}
                                     ${qjson_LIBRARIES}
                                     ${QT_QTDECLARATIVE_LIBRARY})

install(TARGETS muon-installer ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/qml/qml/*.qml")
install( PROGRAMS muon-installer.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES muon-installerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-installer)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
install(FILES config/muon-installer.kcfg DESTINATION ${KCFG_INSTALL_DIR})
