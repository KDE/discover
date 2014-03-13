include_directories(..)

add_executable(knsbackendtest KNSBackendTest.cpp)
add_test(knsbackendtest knsbackendtest)
ecm_mark_as_test(knsbackendtest)
target_link_libraries(knsbackendtest
    muonprivate
    
    Qt5::Test Qt5::Core
    KF5:KNewStuff KF5::KIOWidgets ${LIBATTICA_LIBRARIES}
)
