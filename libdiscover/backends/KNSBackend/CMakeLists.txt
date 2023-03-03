add_subdirectory(tests)

add_library(kns-backend MODULE
    KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp
    KNSTransaction.cpp
)

target_link_libraries(kns-backend Discover::Common AppStreamQt KF6::ConfigCore KF6::Attica KF6::NewStuffCore KF6::WidgetsAddons Qt::Xml)

install(TARGETS kns-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
