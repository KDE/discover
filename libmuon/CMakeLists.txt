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

add_subdirectory(tests)

set(muon_LIB_SRCS
    Category/Category.cpp
    Category/CategoryModel.cpp
    MuonMainWindow.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/AbstractLoginBackend.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
    resources/PackageState.cpp
    resources/AbstractBackendUpdater.cpp
    resources/ResourcesUpdatesModel.cpp
)

if(QAPT_FOUND)
    set(muon_LIB_SRCS ${muon_LIB_SRCS}
        ApplicationBackend/Application.cpp
        ApplicationBackend/ApplicationBackend.cpp
        ChangesDialog.cpp
        MuonStrings.cpp
        QAptActions.cpp
        ApplicationBackend/LaunchListModel.cpp
        ApplicationBackend/ApplicationUpdates.cpp
        HistoryView/HistoryProxyModel.cpp
        HistoryView/HistoryView.cpp
        ReviewsBackend/ReviewsBackend.cpp #TODO: rename to AptReviewsBackend
        ReviewsBackend/UbuntuLoginBackend.cpp
    )
    add_definitions(-DQAPT_ENABLED)
    qt4_add_dbus_interface(muon_LIB_SRCS ReviewsBackend/ubuntu_sso_dbus_interface.xml ubuntu_sso -i "ReviewsBackend/LoginMetaTypes.h")
endif(QAPT_FOUND)

if(LIBATTICA_FOUND)
    set(muon_LIB_SRCS ${muon_LIB_SRCS}
        KNSBackend/KNSBackend.cpp
        KNSBackend/KNSResource.cpp
        KNSBackend/KNSReviews.cpp
        KNSBackend/KNSUpdater.cpp
    )
endif(LIBATTICA_FOUND)

if(BODEGA_FOUND)
    set(muon_LIB_SRCS ${muon_LIB_SRCS}
        BodegaBackend/BodegaBackend.cpp
        BodegaBackend/BodegaResource.cpp
    )
endif(BODEGA_FOUND)

kde4_add_kcfg_files(muon_LIB_SRCS GENERATE_MOC MuonDataSources.kcfgc)
kde4_add_library(muonprivate SHARED ${muon_LIB_SRCS})

set_target_properties(muonprivate PROPERTIES
    VERSION 1.3.65
    SOVERSION 1
)

if(NOT qjson_LIBRARIES) #hack to compatibilize different qjson finder versions
    set(qjson_LIBRARIES ${QJSON_LIBRARIES})
endif()

target_link_libraries(muonprivate
    ${DEBCONF_KDE_LIB}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QAPT_LIBRARY}
    ${qjson_LIBRARIES}
    ${QTOAUTH_LIBRARY}
    ${KDE4_KNEWSTUFF3_LIBS}
)

if(LIBATTICA_FOUND)
    target_link_libraries(muonprivate ${LIBATTICA_LIBRARIES})
endif(LIBATTICA_FOUND)

if(QAPT_FOUND)
    target_link_libraries(muonprivate ${QAPT_LIBRARY})
endif(QAPT_FOUND)

if(BODEGA_FOUND)
    target_link_libraries(muonprivate ${BODEGA_LIBRARIES})
endif(BODEGA_FOUND)

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES categories.xml DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
