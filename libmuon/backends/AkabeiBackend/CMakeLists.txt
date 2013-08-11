include_directories(
${AKABEICORE_INCLUDE_DIR}
${AKABEICLIENT_INCLUDE_DIR}
)

set(akabeibackend_SRCS 
    AkabeiQuestion.cpp
    AkabeiBackend.cpp
    AkabeiResource.cpp
    AkabeiTransaction.cpp
)

kde4_add_plugin(muon-akabeibackend SHARED ${akabeibackend_SRCS})
target_link_libraries(muon-akabeibackend ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${AKABEICLIENT_LIBRARIES} ${AKABEICORE_LIBRARIES} muonprivate
)
install(TARGETS muon-akabeibackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-akabei-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES akabei-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)
