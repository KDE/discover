add_definitions( -DPROJECT_NAME=${PROJECT_NAME} -DPROJECT_VERSION=${PROJECT_VERSION})

set(fwupd-backend_SRCS
    FwupdResource.cpp
    FwupdBackend.cpp
    FwupdTransaction.cpp
    FwupdSourcesBackend.cpp
)

kcoreaddons_add_plugin(fwupd-backend SOURCES ${fwupd-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(fwupd-backend
    Qt::Core
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
    PkgConfig::Fwupd
)
if (Fwupd_VERSION VERSION_LESS 1.5.8)
    target_compile_definitions(fwupd-backend PRIVATE -DFWUPD_EXTERNC_REQUIRED)
endif()

