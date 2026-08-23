#SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
#SPDX-License-Identifier: BSD-3-Clause

add_executable(discover_sysupdate_helper
    helper.cpp
    helper.h
)

target_link_libraries(discover_sysupdate_helper PRIVATE
    Qt::Core
    KF6::AuthCore
    discover_sysupdate_config
)

install(TARGETS discover_sysupdate_helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})

kauth_install_helper_files(discover_sysupdate_helper org.kde.discover.sysupdate root)
kauth_install_actions(org.kde.discover.sysupdate org.kde.discover.sysupdate.actions)
