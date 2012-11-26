set(knsbackend_SRCS
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp
    KNSUpdater.cpp
)

kde4_add_plugin(muon-knsbackend SHARED ${knsbackend_SRCS})
target_link_libraries(muon-knsbackend muonprivate
        ${LIBATTICA_LIBRARIES} ${KDE4_KNEWSTUFF3_LIBS})
