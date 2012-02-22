include_directories(.. ${CMAKE_CURRENT_BINARY_DIR}/..)

set(muon_installer_qml_SRCS
    main.cpp
    MuonInstallerDeclarativeMainWindow.cpp
    ApplicationProxyModelHelper.cpp
    BackendsSingleton.cpp
    ReviewsModel.cpp
    ApplicationUpdates.cpp
    ../Application.cpp
    ../ApplicationBackend.cpp
    ../ApplicationModel/ApplicationModel.cpp
    ../ApplicationModel/ApplicationProxyModel.cpp
    ../CategoryView/Category.cpp
    ../Transaction.cpp
    ../CategoryModel.cpp
    ../ReviewsBackend/Rating.cpp
    ../ReviewsBackend/Review.cpp
    ../ReviewsBackend/ReviewsBackend.cpp
    ../TransactionListener.cpp)

qt4_add_resources(muon_installer_qml_SRCS resources.qrc)

kde4_add_executable(muon-installer-qml ${muon_installer_qml_SRCS})

target_link_libraries(muon-installer-qml muonprivate
                                     ${KDE4_KIO_LIBS}
                                     kdeclarative
                                     ${QAPT_LIBRARY}
                                     ${DEBCONF_KDE_LIB}
                                     ${QZEITGEIST_LIBRARY}
                                     ${qjson_LIBRARIES}
                                     ${QT_QTDECLARATIVE_LIBRARY})

install(TARGETS muon-installer-qml ${INSTALL_TARGETS_DEFAULT_ARGS} )
