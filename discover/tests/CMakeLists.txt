include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ..)

macro(muon_add_unit_test name)
    kde4_add_unit_test(${name} ${ARGN})
    target_link_libraries(${name}
        muonprivate
        
        ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY}
        ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KIO_LIBS} ${QAPT_LIBRARY} attica
    )
endmacro(muon_add_unit_test)

if(QAPT_FOUND)
    add_definitions(-DQAPT_ENABLED)
    muon_add_unit_test(originsbackendtest OriginsBackendTest.cpp ../OriginsBackend.cpp)
    muon_add_unit_test(reviewstest ReviewsTest.cpp ${CMAKE_SOURCE_DIR}/libmuon/backends/ApplicationBackend/tests/modeltest.cpp)
endif(QAPT_FOUND)
