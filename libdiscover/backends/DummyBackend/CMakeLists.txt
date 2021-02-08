add_subdirectory(tests)

set(dummy-backend_SRCS
    DummyResource.cpp
    DummyBackend.cpp
    DummyReviewsBackend.cpp
    DummyTransaction.cpp
    DummySourcesBackend.cpp
)

add_library(dummy-backend MODULE ${dummy-backend_SRCS})
target_link_libraries(dummy-backend Qt::Core Qt::Widgets KF5::CoreAddons KF5::ConfigCore Discover::Common)

install(TARGETS dummy-backend DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover)
install(FILES dummy-backend-categories.xml DESTINATION ${KDE_INSTALL_DATADIR}/libdiscover/categories)

add_library(DummyNotifier MODULE DummyNotifier.cpp)
target_link_libraries(DummyNotifier Discover::Notifiers)
set_target_properties(DummyNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS DummyNotifier DESTINATION ${KDE_INSTALL_PLUGINDIR}/discover-notifier)
