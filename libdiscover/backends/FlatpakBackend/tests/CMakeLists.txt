set(EXTRA_LIBS
    PkgConfig::Flatpak
    libdiscover-backend-flatpak-logging-category
)
add_unit_test(flatpaktest FlatpakTest.cpp)
set_tests_properties(flatpaktest PROPERTIES TIMEOUT 700)
