include_directories(
${AKABEICORE_INCLUDE_DIR}
${AKABEICLIENT_INCLUDE_DIR}
)

set(akabeibackend_SRCS
    AppstreamUtils.cpp
    AppAkabeiResource.cpp
    AkabeiUpdater.cpp 
    AkabeiQuestion.cpp
    AkabeiBackend.cpp
    AkabeiResource.cpp
    AkabeiTransaction.cpp
)

add_library(muon-akabeibackend ${akabeibackend_SRCS})
target_link_libraries(muon-akabeibackend ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${qjson_LIBRARIES} ${AKABEICLIENT_LIBRARIES} ${AKABEICORE_LIBRARIES} DiscoverCommon)
install(TARGETS muon-akabeibackend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-akabei-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-akabei-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

set(akabeinotifier_SRCS
    AkabeiNotifier.cpp
)

add_library(kded_muonakabeinotifier MODULE ${akabeinotifier_SRCS})

target_link_libraries(kded_muonakabeinotifier ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${AKABEICLIENT_LIBRARIES} ${AKABEICORE_LIBRARIES} DiscoverCommon)

install(TARGETS kded_muonakabeinotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muonakabeinotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
