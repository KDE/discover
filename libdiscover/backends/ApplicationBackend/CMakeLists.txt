# we will have our own fork of the library now, because they haven't still made their mind out of Qt5
# find_package(QtOAuth REQUIRED)
add_subdirectory(qoauth)
add_subdirectory(libmuonapt)

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

add_library(qapt-backend MODULE ${appsbackend_SRCS})
target_link_libraries(qapt-backend Qt5::Widgets Qt5::DBus Qt5::Concurrent
     KF5::Archive KF5::KIOWidgets KF5::XmlGui DebconfKDE::Main KF5::IconThemes AppstreamQt
     Muon::QOAuth QApt::Main Discover::Common MuonApt
)
target_include_directories(qapt-backend PRIVATE /usr/include/Qca-qt5/QtCrypto)

install(TARGETS qapt-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES qapt-backend.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
install(FILES qapt-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/libdiscover/applicationsbackend/
    PERMISSIONS
        OWNER_EXECUTE OWNER_READ OWNER_WRITE
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
)

foreach(testing IN ITEMS ON OFF)
    set(name MuonApplicationNotifier)
    set(type MODULE)
    if(${testing})
        set(name MuonApplicationNotifierTestLib)
        set(type STATIC)
    endif()
    add_library(${name} ${type} ApplicationNotifier.cpp)
    target_compile_definitions(${name} PRIVATE -DCMAKE_INSTALL_FULL_LIBEXECDIR_KF5=\"${CMAKE_INSTALL_FULL_LIBEXECDIR_KF5}\")
    target_link_libraries(${name} KF5::CoreAddons KF5::I18n KF5::Notifications KF5::IconThemes Discover::Notifiers)
endforeach()

install(TARGETS MuonApplicationNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)
install(FILES muonapplicationnotifier.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
