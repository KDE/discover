configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-paths.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-paths.h)

ki18n_wrap_ui(SnapMacaroonDialog_SRCS SnapMacaroonDialog.ui)
add_executable(SnapMacaroonDialog
    SnapMacaroonDialog.cpp
    ${SnapMacaroonDialog_SRCS}
)
target_link_libraries(SnapMacaroonDialog
    Qt::Network
    Qt::Widgets
    KF6::AuthCore
    KF6::I18n
)
install(TARGETS SnapMacaroonDialog DESTINATION ${KDE_INSTALL_LIBEXECDIR}/discover)

add_executable(libsnap_helper
    SnapAuthHelper.cpp
)
target_link_libraries(libsnap_helper
    Qt::Network
    KF6::AuthCore
    Snapd::Core
)
install(TARGETS libsnap_helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})

kauth_install_actions(org.kde.discover.libsnapclient org.kde.discover.libsnapclient.actions)
kauth_install_helper_files(libsnap_helper org.kde.discover.libsnapclient root)
