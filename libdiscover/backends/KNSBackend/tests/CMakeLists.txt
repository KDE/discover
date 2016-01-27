include_directories(..)

add_executable(knsbackendtest KNSBackendTest.cpp)
add_test(knsbackendtest knsbackendtest)
ecm_mark_as_test(knsbackendtest)
target_link_libraries(knsbackendtest
    Discover::Common
    
    Qt5::Test Qt5::Core
    KF5::NewStuff KF5::Attica
)
