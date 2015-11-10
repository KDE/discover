function(add_unit_test name)
    add_executable(${name} ${CMAKE_SOURCE_DIR}/libdiscover/backends/ApplicationBackend/tests/modeltest.cpp ${ARGN})
    add_test(${name} ${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        Discover::Common

        Qt5::Test Qt5::Core
        KF5::KIOWidgets KF5::XmlGui ${EXTRA_LIBS})
endfunction()

if(AKABEICLIENT_FOUND)
    add_subdirectory(AkabeiBackend)#TODO: Add checks
endif()

if(KF5Attica_FOUND AND KF5NewStuff_FOUND)
    add_subdirectory(KNSBackend)
endif()

if(QApt_FOUND)
    add_subdirectory(ApplicationBackend)
endif()

if(BODEGA_FOUND)
    add_subdirectory(BodegaBackend)
endif()

if(packagekitqt5_FOUND AND AppstreamQt_FOUND)
    add_subdirectory(PackageKitBackend)
endif()

option(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if(BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif()
