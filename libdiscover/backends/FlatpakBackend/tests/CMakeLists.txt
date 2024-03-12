set(EXTRA_LIBS
    PkgConfig::Flatpak
)
add_unit_test(flatpaktest FlatpakTest.cpp)
set_tests_properties(flatpaktest PROPERTIES TIMEOUT 600)
