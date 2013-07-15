include_directories(..)

kde4_add_unit_test(knsbackendtest KNSBackendTest.cpp)
target_link_libraries(knsbackendtest
    muonprivate
    
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
    ${KDE4_KNEWSTUFF3_LIBS} ${KDE4_KIO_LIBS} ${LIBATTICA_LIBRARIES}
)
