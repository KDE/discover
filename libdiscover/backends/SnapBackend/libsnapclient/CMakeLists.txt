add_library(SnapClient SnapSocket.cpp)
target_link_libraries(SnapClient PUBLIC Qt5::Network KF5::Auth)
add_library(Snap::Client ALIAS SnapClient)
install(TARGETS SnapClient DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})

add_executable(libsnap_helper SnapAuthHelper.cpp)
target_link_libraries(libsnap_helper Qt5::Network KF5::Auth)
install(TARGETS libsnap_helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})

kauth_install_actions(org.kde.discover.libsnapclient org.kde.discover.libsnapclient.actions)
kauth_install_helper_files(libsnap_helper org.kde.discover.libsnapclient root)
