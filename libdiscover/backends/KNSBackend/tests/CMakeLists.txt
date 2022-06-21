include_directories(..)

add_executable(knsbackendtest KNSBackendTest.cpp)
target_link_libraries(knsbackendtest PRIVATE Discover::Common Qt::Core Qt::Test KF5::Attica KF5::NewStuff)
