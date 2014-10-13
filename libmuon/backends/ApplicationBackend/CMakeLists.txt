include_directories(.)

add_subdirectory(tests)

include_directories(${QTOAUTH_INCLUDE_DIR})

set(appsbackend_SRCS
    ApplicationBackend.cpp
    Application.cpp
    ApplicationUpdates.cpp
    ReviewsBackend.cpp #TODO: rename to AptReviewsBackend
    UbuntuLoginBackend.cpp
    AptSourcesBackend.cpp
)

qt5_add_dbus_interface(appsbackend_SRCS ubuntu_sso_dbus_interface.xml ubuntu_sso OPTIONS -i "LoginMetaTypes.h")

add_library(muon-applications-backend MODULE ${appsbackend_SRCS})
target_link_libraries(muon-applications-backend Qt5::Widgets Qt5::DBus Qt5::Concurrent
     KF5::Archive KF5::KIOWidgets KF5::XmlGui DebconfKDE::Main KF5::IconThemes
     ${QTOAUTH_LIBRARY} QApt::Main muonprivate MuonApt
     qca #qtoauth should be dragging it, but it doesn't
)

install(TARGETS muon-applications-backend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-applications-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-applications-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_library(kded_muonapplicationnotifier MODULE ApplicationNotifier.cpp)
target_link_libraries(kded_muonapplicationnotifier KF5::CoreAddons KF5::Notifications KF5::IconThemes MuonNotifiers)

install(TARGETS kded_muonapplicationnotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muonapplicationnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
install(FILES muonapplicationnotifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
