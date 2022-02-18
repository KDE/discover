add_library(DiscoverNotifiers BackendNotifierModule.cpp BackendNotifierModule.h)
target_link_libraries(DiscoverNotifiers
    PUBLIC
        Qt::Core
)

generate_export_header(DiscoverNotifiers)

target_include_directories(DiscoverNotifiers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS DiscoverNotifiers DESTINATION ${KDE_INSTALL_LIBDIR}/plasma-discover)

add_library(Discover::Notifiers ALIAS DiscoverNotifiers)
