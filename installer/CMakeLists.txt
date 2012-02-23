# make the macro a bit more smart, making it possible to pass arguments to qdbusxml2cpp
MACRO(QT4_ADD_DBUS_INTERFACE _sources _interface _basename)
  GET_FILENAME_COMPONENT(_infile ${_interface} ABSOLUTE)
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)

  # handling more arguments (as in FindQt4.cmake from KDE4) will come soon, then
  # _params will be used for more than just -m
  SET(_params -m)

  ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
      COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} ${_params} -p ${_basename} ${_infile} ${ARGN}
      DEPENDS ${_infile})

  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)

  QT4_GENERATE_MOC(${_header} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})

ENDMACRO(QT4_ADD_DBUS_INTERFACE)

add_subdirectory(muon-qml)

if (HAVEKDE4_6_60)
else (HAVEKDE4_6_60)
    set(messagewidget_SRCS backports/kmessagewidget.cpp)
    include_directories(backports)
endif (HAVEKDE4_6_60)

include_directories(../libmuon)

set(muon_installer_SRCS
    main.cpp
    ApplicationLauncher.cpp
    ApplicationWindow.cpp
    ApplicationView/ApplicationDelegate.cpp
    ApplicationView/ApplicationExtender.cpp
    ApplicationView/ApplicationListView.cpp
    ApplicationView/ApplicationViewWidget.cpp
    ApplicationDetailsView/ApplicationDetailsWidget.cpp
    ApplicationDetailsView/ApplicationDetailsView.cpp
    ApplicationDetailsView/AddonsWidget.cpp
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
    ViewSwitcher.cpp
    ReviewsWidget/ReviewWidget.cpp
    ReviewsWidget/ReviewsWidget.cpp
    ScreenShotOverlay.cpp
    ${messagewidget_SRCS})
    
qt4_add_dbus_interface(muon_installer_SRCS ReviewsBackend/ubuntu_sso_dbus_interface.xml ubuntu_sso -i "ReviewsBackend/LoginMetaTypes.h")

kde4_add_kcfg_files(muon_installer_SRCS GENERATE_MOC config/MuonInstallerSettings.kcfgc)

kde4_add_executable(muon-installer ${muon_installer_SRCS})

target_link_libraries(muon-installer muonprivate
                                     ${KDE4_KIO_LIBS}
                                     ${QAPT_LIBRARY}
                                     ${QZEITGEIST_LIBRARY}
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
