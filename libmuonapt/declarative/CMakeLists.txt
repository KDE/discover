project(muonapt-declarative)

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ..)

add_library(muonaptdeclarativeplugin MuonAptDeclarativePlugin.cpp OriginsBackend.cpp)

target_link_libraries(muonaptdeclarativeplugin Qt5::Core Qt5::Widgets Qt5::Quick ${KDE4_KDEUI_LIBRARY} ${QAPT_LIBRARY} muonprivate muonapt)

install(TARGETS muonaptdeclarativeplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/muonapt)
install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/muonapt)

add_subdirectory(tests)
