set(muon-notifier_SRCS
    MuonNotifier.cpp
    event.cpp
    configwatcher.cpp
    distupgradeevent/distupgradeevent.cpp
    UpdateEvent/UpdateEvent.cpp
)

qt4_add_dbus_adaptor(muon-notifier_SRCS org.kubuntu.MuonNotifier.xml
                     configwatcher.h ConfigWatcher)

kde4_add_plugin(kded_muon-notifier ${muon-notifier_SRCS})

target_link_libraries(kded_muon-notifier ${KDE4_KIO_LIBS})

install(TARGETS kded_muon-notifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muon-notifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
install(FILES muon-notifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
