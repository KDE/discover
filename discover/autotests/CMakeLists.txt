ecm_add_test(IconColorsTest.cpp ../IconColors.cpp TEST_NAME IconColorsTest LINK_LIBRARIES Qt5::Test Qt5::Gui)
ecm_add_test(PaginateModelTest.cpp ../PaginateModel.cpp ${CMAKE_SOURCE_DIR}/libdiscover/tests/modeltest.cpp TEST_NAME PaginateModelTest LINK_LIBRARIES Qt5::Test Qt5::Gui)
target_include_directories(PaginateModelTest PUBLIC ${CMAKE_SOURCE_DIR}/libdiscover/)

if(BUILD_DummyBackend)
    add_test(NAME toplevels COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/toplevels.qml")
endif()
