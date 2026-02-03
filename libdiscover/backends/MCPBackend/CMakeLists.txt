# SPDX-FileCopyrightText: 2025 Yakup Atahanov
# SPDX-License-Identifier: BSD-3-Clause

set(mcp-backend_SRCS
    MCPResource.cpp
    MCPBackend.cpp
    MCPTransaction.cpp
)

kcoreaddons_add_plugin(mcp-backend SOURCES ${mcp-backend_SRCS} INSTALL_NAMESPACE "discover")

target_link_libraries(mcp-backend
    Qt::Core
    Qt::Network
    KF6::CoreAddons
    KF6::I18n
    Discover::Common
)

install(FILES mcp-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)
