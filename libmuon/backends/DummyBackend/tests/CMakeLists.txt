include_directories(.. ../../ApplicationBackend/tests)

macro(libmuon_add_unit_test name)
    add_executable(${name} ../../ApplicationBackend/tests/modeltest.cpp ${ARGN})
    add_test(${name} ${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        MuonCommon

        Qt5::Test Qt5::Core
        KF5::KIOWidgets)
endmacro(libmuon_add_unit_test)

libmuon_add_unit_test(dummytest DummyTest.cpp)
