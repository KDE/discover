function(add_unit_test name)
    add_executable(${name} ${CMAKE_SOURCE_DIR}/libdiscover/tests/modeltest.cpp ${ARGN})
    add_test(${name} ${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        Discover::Common

        Qt5::Test Qt5::Core ${EXTRA_LIBS})
endfunction()

if(KF5Attica_FOUND AND KF5NewStuff_FOUND)
   add_subdirectory(KNSBackend)
endif()

if(packagekitqt5_FOUND AND AppStreamQt_FOUND)
    add_subdirectory(PackageKitBackend)
endif()

option(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if(BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif()

option(BUILD_FlatpakBackend "Build Flatpak support" "ON")
if(FLATPAK_FOUND AND AppStreamQt_FOUND AND BUILD_FlatpakBackend)
    add_subdirectory(FlatpakBackend)
elseif(BUILD_FlatpakBackend)
    message(WARNING "BUILD_FlatpakBackend enabled but Flatpak=${FLATPAK_FOUND} or AppStreamQt=${AppStreamQt_FOUND} not found")
endif()

option(BUILD_SnapBackend "Build Snap support." "ON")
if(BUILD_SnapBackend)
    find_package(Snapd)
    set_package_properties(Snapd PROPERTIES
        DESCRIPTION "Library that exposes Snapd"
        URL "http://www.snapcraft.io"
        PURPOSE "Required to build the Snap backend"
        TYPE OPTIONAL)

    add_subdirectory(SnapBackend)
endif()
