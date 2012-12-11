project(muon-declarative)

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ..)

kde4_add_library(muondeclarativeplugin SHARED MuonDeclarativePlugin.cpp ApplicationProxyModelHelper.cpp)

target_link_libraries(muondeclarativeplugin ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY} muonprivate)

install(TARGETS muondeclarativeplugin DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/muon)
install(FILES qmldir DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/muon)
