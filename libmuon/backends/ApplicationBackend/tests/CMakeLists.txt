include_directories(..)

macro(libmuon_add_unit_test name)
    kde4_add_unit_test(${name}  modeltest.cpp ${ARGN})
    target_link_libraries(${name}
        muonprivate muonapt
        
        ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
        ${KDE4_KIO_LIBS} ${QAPT_LIBRARY})
endmacro(libmuon_add_unit_test)

install(FILES rnrtestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libmuon)
install(FILES popcontestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libmuon)

libmuon_add_unit_test(applicationbackendtest ApplicationBackendTest.cpp)
libmuon_add_unit_test(reviewstest ReviewsTest.cpp)
