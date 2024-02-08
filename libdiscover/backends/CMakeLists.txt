function(add_unit_test name)
    add_executable(${name} ${ARGN})
    add_test(${name} dbus-run-session ${CMAKE_BINARY_DIR}/bin/${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        Discover::Common
        Qt::Test
        Qt::Core
        ${EXTRA_LIBS}
    )
endfunction()

# Shared logging category for backends which don't declare their own ones
ecm_qt_declare_logging_category(libdiscover-backend-logging-category_SRCS
    HEADER libdiscover_backend_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend
    DESCRIPTION "libdiscover (backend)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-logging-category STATIC
    ${libdiscover-backend-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-logging-category
    PRIVATE
        Qt::Core
)

if (KF6Attica_FOUND AND KF6NewStuff_FOUND AND AppStreamQt_FOUND)
   add_subdirectory(KNSBackend)
endif()

if (packagekitqt6_FOUND AND AppStreamQt_FOUND)
    add_subdirectory(PackageKitBackend)
endif()

option(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if (BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif()

option(BUILD_FlatpakBackend "Build Flatpak support" "ON")
if (Flatpak_FOUND AND AppStreamQt_FOUND AND BUILD_FlatpakBackend)
    add_subdirectory(FlatpakBackend)
elseif (BUILD_FlatpakBackend)
    message(WARNING "BUILD_FlatpakBackend enabled but Flatpak=${Flatpak_FOUND} or AppStreamQt=${AppStreamQt_FOUND} not found")
endif()

find_package(Snapd)
set_package_properties(Snapd PROPERTIES
    DESCRIPTION "Library that exposes Snapd"
    URL "https://www.snapcraft.io"
    PURPOSE "Required to build the Snap backend"
    TYPE OPTIONAL
)

option(BUILD_SteamOSBackend "Build SteamOS support." "OFF")
if (BUILD_SteamOSBackend)
    add_subdirectory(SteamOSBackend)
endif()

option(BUILD_SnapBackend "Build Snap support." "ON")
if (BUILD_SnapBackend AND AppStreamQt_FOUND AND Snapd_FOUND)
    add_subdirectory(SnapBackend)
endif()

option(BUILD_FwupdBackend "Build Fwupd support." "ON")
if (BUILD_FwupdBackend AND TARGET PkgConfig::Fwupd)
    add_subdirectory(FwupdBackend)
endif()

option(BUILD_RpmOstreeBackend "Build rpm-ostree support." "ON")
if (BUILD_RpmOstreeBackend AND Ostree_FOUND AND RpmOstree_FOUND)
    add_subdirectory(RpmOstreeBackend)
endif()
