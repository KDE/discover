include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ..)

add_library(muondeclarativeplugin MuonDeclarativePlugin.cpp ApplicationProxyModelHelper.cpp)

target_link_libraries(muondeclarativeplugin Qt5::Core Qt5::Qml MuonCommon)

install(TARGETS muondeclarativeplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/muon)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/muon)
