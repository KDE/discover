include_directories(..)

macro(libmuon_add_unit_test name)
    kde4_add_unit_test(${name}  modeltest.cpp ${ARGN})
    target_link_libraries(${name}
        muonprivate
        
        ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
        ${KDE4_KIO_LIBS} ${QAPT_LIBRARY} muon-appsbackend
    )
endmacro(libmuon_add_unit_test)

libmuon_add_unit_test(resourcesmodeltest ResourcesModelTest.cpp)
libmuon_add_unit_test(reviewstest ReviewsTest.cpp)
