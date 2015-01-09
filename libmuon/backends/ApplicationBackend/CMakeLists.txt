# we will have our own fork of the library now, because they haven't still made their mind out of Qt5
# find_package(QtOAuth REQUIRED)
add_subdirectory(qoauth)

include_directories(.)

add_subdirectory(tests)

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
     Muon::QOAuth QApt::Main muonprivate MuonApt
)

install(TARGETS muon-applications-backend DESTINATION ${PLUGIN_INSTALL_DIR}/muon)
install(FILES muon-applications-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libmuon/backends)
install(FILES muon-applications-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_library(MuonApplicationNotifier MODULE ApplicationNotifier.cpp)
target_link_libraries(MuonApplicationNotifier KF5::CoreAddons KF5::I18n KF5::Notifications KF5::IconThemes MuonNotifiers)

install(TARGETS MuonApplicationNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/muon-notifier)
install(FILES muonapplicationnotifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
