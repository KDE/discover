add_definitions( -DPROJECT_NAME=${PROJECT_NAME} -DPROJECT_VERSION=${PROJECT_VERSION})

set(fwupd-backend_SRCS
    FwupdResource.cpp
    FwupdBackend.cpp
    FwupdTransaction.cpp
    FwupdSourcesBackend.cpp
)

add_library(fwupd-backend MODULE ${fwupd-backend_SRCS})
target_link_libraries(fwupd-backend Qt5::Core KF5::CoreAddons KF5::ConfigCore Discover::Common PkgConfig::Fwupd)

install(TARGETS fwupd-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)

