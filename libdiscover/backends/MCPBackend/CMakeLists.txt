# SPDX-FileCopyrightText: 2025 Yakup Atahanov
# SPDX-License-Identifier: BSD-3-Clause

ecm_qt_declare_logging_category(libdiscover-backend-mcp-logging-category_SRCS
    HEADER libdiscover_backend_mcp_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_MCP_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.mcp
    DESCRIPTION "libdiscover (backend - MCP)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-mcp-logging-category STATIC
    ${libdiscover-backend-mcp-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-mcp-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-mcp-logging-category
    PRIVATE
        Qt::Core
)

set(mcp-backend_SRCS
    MCPResource.cpp
    MCPBackend.cpp
    MCPTransaction.cpp
)

kcoreaddons_add_plugin(mcp-backend SOURCES ${mcp-backend_SRCS} INSTALL_NAMESPACE "discover")

target_link_libraries(mcp-backend
    PRIVATE
        Qt::Core
        Qt::Network
        KF6::CoreAddons
        KF6::I18n
        Discover::Common
        libdiscover-backend-mcp-logging-category
)

install(FILES mcp-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)
