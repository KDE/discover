add_executable(IconColorsTest IconColorsTest.cpp ../IconColors.cpp)
target_link_libraries(IconColorsTest Qt5::Test Qt5::Gui)

add_executable(PaginateModelTest PaginateModelTest.cpp ../PaginateModel.cpp ${CMAKE_SOURCE_DIR}/libdiscover/tests/modeltest.cpp)
target_link_libraries(PaginateModelTest Qt5::Test Qt5::Gui)
target_include_directories(PaginateModelTest PUBLIC ${CMAKE_SOURCE_DIR}/libdiscover/)

if(BUILD_DummyBackend)
    add_test(NAME toplevels COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/toplevels.qml")
endif()
