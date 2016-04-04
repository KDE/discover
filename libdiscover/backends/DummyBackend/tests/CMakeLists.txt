include_directories(.. ../../ApplicationBackend/tests)

add_unit_test(dummytest DummyTest.cpp)
add_unit_test(updatedummytest UpdateDummyTest.cpp)

target_link_libraries(updatedummytest KF5::CoreAddons)
