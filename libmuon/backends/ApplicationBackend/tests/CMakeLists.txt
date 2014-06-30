include_directories(..)

macro(libmuon_add_unit_test name)
    add_executable(${name}  modeltest.cpp ${ARGN})
    add_test(${name} ${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        muonprivate muonapt
        
        Qt5::Test Qt5::Core
        KF5::KIOWidgets ${QAPT_LIBRARY})
endmacro(libmuon_add_unit_test)

install(FILES rnrtestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES popcontestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libmuon)

libmuon_add_unit_test(applicationbackendtest ApplicationBackendTest.cpp)
libmuon_add_unit_test(reviewstest ReviewsTest.cpp)
