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
    Application.cpp
    ApplicationBackend.cpp
    ApplicationModel/ApplicationModel.cpp
    ApplicationModel/ApplicationProxyModel.cpp
    ApplicationModel/LaunchListModel.cpp
    ChangesDialog.cpp
    MuonMainWindow.cpp
    MuonStrings.cpp
    Category/Category.cpp
    Category/CategoryModel.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
    mobile/src/mousecursor.cpp
    ReviewsBackend/AbstractReviewsBackend.cpp
    ReviewsBackend/Rating.cpp
    ReviewsBackend/Review.cpp
    ReviewsBackend/ReviewsBackend.cpp
    ReviewsBackend/AbstractLoginBackend.cpp
    ReviewsBackend/UbuntuLoginBackend.cpp
    settings/SettingsPageBase.cpp
    settings/NotifySettingsPage.cpp
    Transaction/Transaction.cpp
    Transaction/TransactionListener.cpp
    Transaction/TransactionModel.cpp
    resources/ResourcesModel.cpp
    resources/ResourcesProxyModel.cpp
    resources/AbstractResourcesBackend.cpp
    resources/AbstractResource.cpp
)
qt4_add_dbus_interface(muon_LIB_SRCS ReviewsBackend/ubuntu_sso_dbus_interface.xml ubuntu_sso -i "ReviewsBackend/LoginMetaTypes.h")

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
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_PHONON_LIBS}
    ${KDE4_SOLID_LIBS}
    ${QAPT_LIBRARY}
    ${qjson_LIBRARIES}
    ${QTOAUTH_LIBRARY}
)
    
if (ENABLE_QZEITGEIST)
    target_link_libraries(muonprivate ${QZEITGEIST_LIBRARY})
endif (ENABLE_QZEITGEIST)

file(GLOB muon_QMLS "${CMAKE_CURRENT_SOURCE_DIR}/mobile/qml/*.qml")

install(TARGETS muonprivate ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES moo.ogg DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES ${muon_QMLS} DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES categories.xml DESTINATION ${DATA_INSTALL_DIR}/muon-installer)
