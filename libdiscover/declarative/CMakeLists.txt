include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ..)

add_library(discoverdeclarativeplugin DiscoverDeclarativePlugin.cpp ApplicationProxyModelHelper.cpp)

target_link_libraries(discoverdeclarativeplugin Qt5::Core Qt5::Qml Discover::Common)

install(TARGETS discoverdeclarativeplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/discover)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/discover)
