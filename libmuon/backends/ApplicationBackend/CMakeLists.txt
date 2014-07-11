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

include_directories(. ${CMAKE_SOURCE_DIR}/libmuonapt)

add_subdirectory(tests)

set(appsbackend_SRCS
    ApplicationBackend.cpp
    Application.cpp
    ApplicationUpdates.cpp
    ReviewsBackend.cpp #TODO: rename to AptReviewsBackend
    UbuntuLoginBackend.cpp
)

qt5_add_dbus_interface(appsbackend_SRCS ubuntu_sso_dbus_interface.xml ubuntu_sso -i "LoginMetaTypes.h")

add_library(muon-appsbackend MODULE ${appsbackend_SRCS})
target_link_libraries(muon-appsbackend Qt5::Widgets Qt5::DBus Qt5::Concurrent
     KF5::KIOWidgets DebconfKDE::Main
     ${QTOAUTH_LIBRARY} ${QAPT_LIBRARY} muonprivate muonapt
)
install(TARGETS muon-appsbackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-applications-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES apps-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_library(kded_muonapplicationnotifier MODULE ApplicationNotifier.cpp)
target_link_libraries(kded_muonapplicationnotifier ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} muonprivate)
target_include_directories(kded_muonapplicationnotifier PRIVATE ${CMAKE_SOURCE_DIR}/kded)

install(TARGETS kded_muonapplicationnotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muonapplicationnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
install(FILES muonapplicationnotifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
