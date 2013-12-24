project(muon-declarative)

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ..)

add_library(muondeclarativeplugin SHARED MuonDeclarativePlugin.cpp ApplicationProxyModelHelper.cpp)

target_link_libraries(muondeclarativeplugin Qt5::Core Qt5::Widgets Qt5::Qml muonprivate)

install(TARGETS muondeclarativeplugin DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/muon)
install(FILES qmldir DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/muon)
