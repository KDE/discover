set(pkbackend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
)

kde4_add_plugin(muon-pkbackend SHARED ${pkbackend_SRCS})
target_link_libraries(muon-pkbackend muonprivate ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${BODEGA_LIBRARIES})

install(TARGETS muon-pkbackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-packagekit-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
