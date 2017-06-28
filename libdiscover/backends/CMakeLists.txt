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
if(FLATPAK_FOUND AND AppStreamQt_FOUND AND APPSTREAM_FOUND AND BUILD_FlatpakBackend)
    add_subdirectory(FlatpakBackend)
endif()

option(BUILD_SnapBackend "Build Snap support. Still a proof of concept" "OFF")
if(BUILD_SnapBackend)
    add_subdirectory(SnapBackend)
endif()
