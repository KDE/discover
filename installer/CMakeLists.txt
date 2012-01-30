add_subdirectory(muon-qml)

if (HAVEKDE4_6_60)
else (HAVEKDE4_6_60)
    set(messagewidget_SRCS backports/kmessagewidget.cpp)
    include_directories(backports)
endif (HAVEKDE4_6_60)

set(muon_installer_SRCS
    main.cpp
    Application.cpp
    ApplicationBackend.cpp
    ApplicationLauncher.cpp
    ApplicationWindow.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationProxyModel.cpp
    ApplicationModel/ApplicationDelegate.cpp
    ApplicationModel/ApplicationExtender.cpp
    ApplicationModel/ApplicationListView.cpp
    ApplicationModel/ApplicationViewWidget.cpp
    ApplicationDetailsView/ApplicationDetailsWidget.cpp
    ApplicationDetailsView/ApplicationDetailsView.cpp
    ApplicationDetailsView/AddonsWidget.cpp
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
    ScreenShotOverlay.cpp
    Transaction.cpp
    CategoryModel.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/ReviewWidget.cpp
    ReviewsBackend/ReviewsWidget.cpp
    ReviewsBackend/ReviewsBackend.cpp
    ${messagewidget_SRCS})


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/HaveQZeitgeist.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/HaveQZeitgeist.h)

kde4_add_kcfg_files(muon_installer_SRCS GENERATE_MOC config/MuonInstallerSettings.kcfgc)

kde4_add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer muonprivate
                                     ${KDE4_KIO_LIBS}
                                     ${QAPT_LIBRARY}
                                     ${DEBCONF_KDE_LIB}
                                     ${QZEITGEIST_LIBRARY}
                                     ${QJSON_LIBRARIES}
                                     ${QT_QTDECLARATIVE_LIBRARY})

if (ENABLE_QZEITGEIST)
    target_link_libraries(muon-installer ${QZEITGEIST_LIBRARY})
endif (ENABLE_QZEITGEIST)

install(TARGETS muon-installer ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS muon-installer.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
install(FILES muon-installerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon-installer)
install(FILES categories.xml DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
install(FILES config/muon-installer.kcfg DESTINATION ${KCFG_INSTALL_DIR})
