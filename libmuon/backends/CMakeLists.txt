if(AKABEICLIENT_FOUND)
    add_subdirectory(AkabeiBackend)#TODO: Add checks
endif()

if(LIBATTICA_FOUND)
    add_subdirectory(KNSBackend)
endif()

if(BODEGA_FOUND)
    add_subdirectory(BodegaBackend)
endif()

if(QAPT_FOUND)
    add_subdirectory(ApplicationBackend)
elseif(PACKAGEKITQT2_FOUND)
    add_subdirectory(PackageKitBackend)
endif()

option(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if(BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif()
