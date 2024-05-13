if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(dummy-backend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
    DummySourcesBackend.cpp
)

kcoreaddons_add_plugin(dummy-backend SOURCES ${dummy-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(dummy-backend
    Qt::Core
    Qt::Widgets
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)

install(FILES dummy-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

add_library(DummyNotifier MODULE DummyNotifier.cpp)
target_link_libraries(DummyNotifier
    Discover::Notifiers
)
set_target_properties(DummyNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DummyNotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)
