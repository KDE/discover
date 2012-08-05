include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ..)

macro(muon_add_unit_test name)
    kde4_add_unit_test(${name} ${ARGN}
        ../../libmuon/KNSBackend/KNSBackend.cpp ../../libmuon/KNSBackend/KNSResource.cpp ../../libmuon/KNSBackend/KNSReviews.cpp ../../libmuon/KNSBackend/KNSUpdater.cpp

        ../OriginsBackend.cpp ../BackendsSingleton.cpp
        ../ReviewsModel.cpp)
    target_link_libraries(${name}
        muonprivate
        
        ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
        ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KIO_LIBS} ${QAPT_LIBRARY} attica
    )
endmacro(muon_add_unit_test)

if(QAPT_FOUND)
    add_definitions(-DQAPT_ENABLED)
endif(QAPT_FOUND)
muon_add_unit_test(originsbackendtest OriginsBackendTest.cpp)
muon_add_unit_test(reviewstest ReviewsTest.cpp ${CMAKE_SOURCE_DIR}/libmuon/tests/modeltest.cpp)
muon_add_unit_test(knsbackendtest KNSBackendTest.cpp )
