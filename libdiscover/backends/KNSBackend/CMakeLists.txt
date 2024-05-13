if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(kns-backend_SRCS 
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp
    KNSTransaction.cpp
)

kcoreaddons_add_plugin(kns-backend SOURCES ${kns-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(kns-backend
    Discover::Common
    AppStreamQt
    KF6::ConfigCore
    KF6::Attica
    KF6::NewStuffCore
    KF6::WidgetsAddons
    Qt::Xml
)
