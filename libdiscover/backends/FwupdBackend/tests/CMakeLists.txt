#add_unit_test(fwupdtest FwupdTest.cpp)
add_unit_test(updatefwupdtest UpdateFwupdTest.cpp)

target_link_libraries(updatefwupdtest KF5::CoreAddons)
