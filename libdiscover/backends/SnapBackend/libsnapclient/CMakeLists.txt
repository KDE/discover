add_library(SnapClient SnapSocket.cpp)
target_link_libraries(SnapClient PUBLIC Qt5::Core Qt5::Network)
add_library(Snap::Client ALIAS SnapClient)

add_executable(libsnappy_helper SnapAuthHelper.cpp)
target_link_libraries(libsnappy_helper Qt5::Core KF5::Auth)

install(TARGETS libsnappy_helper DESTINATION ${LIBEXEC_INSTALL_DIR})
kauth_install_actions(org.kde.discover.libsnapclient org.kde.discover.libsnapclient.action)
kauth_install_helper_files(libsnappy_helper org.kde.discover.libsnapclient root)

