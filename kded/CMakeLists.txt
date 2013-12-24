set(muon-notifier_SRCS
    MuonNotifier.cpp
    event.cpp
    configwatcher.cpp
    distupgradeevent/distupgradeevent.cpp
    UpdateEvent/UpdateEvent.cpp
)

QT4_ADD_DBUS_ADAPTOR(muon-notifier_SRCS org.kubuntu.MuonNotifier.xml
                     configwatcher.h ConfigWatcher)

add_library(kded_muon-notifier MODULE ${muon-notifier_SRCS})

target_link_libraries(kded_muon-notifier KF5::KIOWidgets)

install(TARGETS kded_muon-notifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muon-notifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
install(FILES muon-notifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muon-notifier)
