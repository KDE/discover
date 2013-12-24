set(bodegabackend_SRCS
    BodegaBackend.cpp
    BodegaResource.cpp
)

add_library(muon-bodegabackend MODULE SHARED ${bodegabackend_SRCS})
target_link_libraries(muon-bodegabackend muonprivate Qt5::Core  ${KDE4_KDEUI_LIBS} ${BODEGA_LIBRARIES})

install(TARGETS muon-bodegabackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-bodegawallpapers-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES bodegawallpapers-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)