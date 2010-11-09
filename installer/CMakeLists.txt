set(muon_installer_SRCS
    main.cpp
    Application.cpp
    ApplicationWindow.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationProxyModel.cpp
    ApplicationModel/ApplicationDelegate.cpp
    ApplicationModel/ApplicationView.cpp
    AvailableView.cpp
    CategoryView.cpp
    CategoryDrawer.cpp
    OriginView.cpp)

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
# install( FILES muon-installerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )
