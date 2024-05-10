set(EXTRA_LIBS
    libdiscover-backend-steamos-logging-category
)
add_unit_test(steamostest SteamOSTest.cpp atomupd1_adaptor.cpp mock_server.cpp )
set_tests_properties(steamostest PROPERTIES TIMEOUT 600)

target_link_libraries(steamostest
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)