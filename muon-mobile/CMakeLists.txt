include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/.. ../libmuon)

set(muon_installer_qml_SRCS
    main.cpp
    MuonInstallerMainWindow.cpp
    ApplicationProxyModelHelper.cpp
    BackendsSingleton.cpp
    ReviewsModel.cpp
    ApplicationUpdates.cpp)

qt4_add_resources(muon_installer_qml_SRCS resources.qrc)

kde4_add_executable(muon-installer-qml ${muon_installer_qml_SRCS})

target_link_libraries(muon-installer-qml muonprivate
                                     ${KDE4_KIO_LIBS}
                                     kdeclarative
                                     ${QAPT_LIBRARY}
                                     ${QZEITGEIST_LIBRARY}
                                     ${QT_QTDECLARATIVE_LIBRARY})

install(TARGETS muon-installer-qml ${INSTALL_TARGETS_DEFAULT_ARGS} )
