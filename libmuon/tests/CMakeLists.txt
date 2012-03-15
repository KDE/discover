include_directories(..)

kde4_add_unit_test(applicationmodeltest ApplicationModelTest.cpp modeltest.cpp)
target_link_libraries(applicationmodeltest
    muonprivate
    
    ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY}
    ${KDE4_KIO_LIBS} ${QAPT_LIBRARY}
)
