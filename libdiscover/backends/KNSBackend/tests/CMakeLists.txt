include_directories(..)

add_executable(knsbackendtest KNSBackendTest.cpp)
target_link_libraries(knsbackendtest PRIVATE Discover::Common Qt::Core Qt::Test KF6::Attica KF6::NewStuffCore)
