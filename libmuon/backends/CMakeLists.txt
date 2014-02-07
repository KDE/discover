if(AKABEICLIENT_FOUND)
    add_subdirectory(AkabeiBackend)#TODO: Add checks
endif(AKABEICLIENT_FOUND)

if(LIBATTICA_FOUND)
    add_subdirectory(KNSBackend)
endif(LIBATTICA_FOUND)

if(QAPT_FOUND)
    add_subdirectory(ApplicationBackend)
endif(QAPT_FOUND)

if(BODEGA_FOUND)
    add_subdirectory(BodegaBackend)
endif(BODEGA_FOUND)

if(PACKAGEKITQT2_FOUND)
    add_subdirectory(PackageKitBackend)
endif(PACKAGEKITQT2_FOUND)

OPTION(BUILD_DummyBackend "Build the DummyBackend" "OFF")
if(BUILD_DummyBackend)
    add_subdirectory(DummyBackend)
endif(BUILD_DummyBackend)
