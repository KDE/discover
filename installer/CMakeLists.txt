set(muon_installer_SRCS
    main.cpp
    Application.cpp
    ApplicationWindow.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationDelegate.cpp
    CategoryView.cpp
    CategoryDrawer.cpp
    OriginView.cpp)

kde4_add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer ${KDE4_KDEUI_LIBS}
                                     ${KDE4_KIO_LIBS}
                                     ${KDE4_SOLID_LIBS}
                                     ${QAPT_LIBRARY}
                                     libmuon
                                     debconf-kde)

install(TARGETS muon-installer ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

#FIXME
# install( FILES muon-installer.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
# install( FILES muon-installerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/muon )