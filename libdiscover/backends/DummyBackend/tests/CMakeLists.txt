add_unit_test(dummytest
    DummyTest.cpp
)

add_unit_test(updatedummytest
    UpdateDummyTest.cpp
)
target_link_libraries(updatedummytest
    KF6::CoreAddons
)

add_test(NAME headless-updates
         COMMAND Plasma::Discover --backends dummy --headless-update)

# Must exit on its own when there is nothing to update
add_test(NAME headless-updates-none
         COMMAND Plasma::Discover --backends dummy --headless-update)
set_tests_properties(headless-updates-none PROPERTIES
                     ENVIRONMENT DISCOVER_DUMMY_NO_UPDATES=1
                     TIMEOUT 30)
