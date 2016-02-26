add_executable(IconColorsTest IconColorsTest.cpp ../IconColors.cpp)
target_link_libraries(IconColorsTest Qt5::Test Qt5::Gui)

if(BUILD_DummyBackend)
    add_test(NAME toplevels COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/toplevels.qml")
endif()
