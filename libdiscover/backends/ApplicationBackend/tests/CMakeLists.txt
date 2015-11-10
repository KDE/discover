include_directories(..)

set(EXTRA_LIBS QApt::Main MuonApt)

install(FILES rnrtestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libdiscover)
install(FILES popcontestratings.txt DESTINATION ${DATA_INSTALL_DIR}/libdiscover)

add_unit_test(applicationbackendtest ApplicationBackendTest.cpp)
add_unit_test(reviewstest ReviewsTest.cpp)
add_unit_test(sourcestest SourcesTest.cpp)
