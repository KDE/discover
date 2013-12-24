include_directories(..)

macro(libmuon_add_unit_test name)
    kde4_add_unit_test(${name}  modeltest.cpp ${ARGN})
    target_link_libraries(${name}
        muonprivate
        
        Qt5::Test Qt5::Core
        KF5::KIOWidgets ${QAPT_LIBRARY})
endmacro(libmuon_add_unit_test)

libmuon_add_unit_test(applicationbackendtest ApplicationBackendTest.cpp)
libmuon_add_unit_test(reviewstest ReviewsTest.cpp)
