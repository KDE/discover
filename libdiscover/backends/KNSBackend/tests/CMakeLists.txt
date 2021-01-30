include_directories(..)

ecm_add_test(KNSBackendTest.cpp TEST_NAME knsbackendtest LINK_LIBRARIES Discover::Common Qt::Core Qt::Test KF5::Attica KF5::NewStuff)
