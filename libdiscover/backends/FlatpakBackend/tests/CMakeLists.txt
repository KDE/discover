set(EXTRA_LIBS libdiscover-backend-flatpak-logging-category)
add_unit_test(flatpaktest FlatpakTest.cpp)
set_tests_properties(flatpaktest PROPERTIES TIMEOUT 600)
