include_directories(..)

add_executable(packagekitbackendtest PackageKitBackendTest.cpp)
add_test(packagekitbackendtest packagekitbackendtest)
ecm_mark_as_test(packagekitbackendtest)
target_link_libraries(packagekitbackendtest
    muonprivate
    
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KIO_LIBS} muon-pkbackend
)
