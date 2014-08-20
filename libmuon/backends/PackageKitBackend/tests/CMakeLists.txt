include_directories(..)

add_executable(packagekitbackendtest PackageKitBackendTest.cpp ../PackageKitUtils.cpp)
add_test(packagekitbackendtest packagekitbackendtest)
ecm_mark_as_test(packagekitbackendtest)
target_link_libraries(packagekitbackendtest
    muonprivate
    
    Qt5::Test Qt5::Core KF5::KIOWidgets PK::packagekitqt5
)
