include_directories(..)

kde4_add_unit_test(packagekitbackendtest PackageKitBackendTest.cpp)
target_link_libraries(packagekitbackendtest
    muonprivate
    
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KIO_LIBS} muon-pkbackend
)
