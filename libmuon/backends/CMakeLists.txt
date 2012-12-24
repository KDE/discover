if(LIBATTICA_FOUND)
    add_subdirectory(KNSBackend)
endif(LIBATTICA_FOUND)

if(QAPT_FOUND)
    add_subdirectory(ApplicationBackend)
endif(QAPT_FOUND)

if(BODEGA_FOUND)
    add_subdirectory(BodegaBackend)
endif(BODEGA_FOUND)

add_subdirectory(PackageKitBackend)
