if (NOT DEFINED ENV{KDECI_BUILD})
    add_unit_test(flatpaktest FlatpakTest.cpp)
endif()
