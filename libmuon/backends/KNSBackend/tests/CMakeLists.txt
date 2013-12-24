include_directories(..)

kde4_add_unit_test(knsbackendtest KNSBackendTest.cpp)
target_link_libraries(knsbackendtest
    muonprivate
    
    Qt5::Test Qt5::Core
    KF5:KNewStuff KF5::KIOWidgets ${LIBATTICA_LIBRARIES}
)
