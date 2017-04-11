ecm_add_test(IconColorsTest.cpp ../IconColors.cpp TEST_NAME IconColorsTest LINK_LIBRARIES Qt5::Test Qt5::Gui)
ecm_add_test(PaginateModelTest.cpp ../PaginateModel.cpp ${CMAKE_SOURCE_DIR}/libdiscover/tests/modeltest.cpp TEST_NAME PaginateModelTest LINK_LIBRARIES Qt5::Test Qt5::Gui)
target_include_directories(PaginateModelTest PUBLIC ${CMAKE_SOURCE_DIR}/libdiscover/)

if(BUILD_DummyBackend)
    add_test(NAME toplevels COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/toplevels.qml")
    add_test(NAME install COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/install.qml")

    add_test(NAME appstreamUrl COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/appstreamUrl.qml" "dummy://techie1")
    add_test(NAME missingResource COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/missingResource.qml" "dummy://caca")
    add_test(NAME apparg COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/appstreamUrl.qml" --application "dummy://techie1")
    add_test(NAME categoryarg COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/categoryArg.qml" --category "dummy 2.1")
    add_test(NAME wrongInput COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/wrongInput.qml" "CMakeLists.txt")
    add_test(NAME packageArgument COMMAND Plasma::Discover --test "${CMAKE_CURRENT_SOURCE_DIR}/packageArgument.qml"
                                                           --local-filename "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
endif()
