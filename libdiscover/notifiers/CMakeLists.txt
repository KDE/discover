add_library(DiscoverNotifiers BackendNotifierModule.cpp)
target_link_libraries(DiscoverNotifiers
    PUBLIC
        Qt5::Core
)

generate_export_header(DiscoverNotifiers)

target_include_directories(DiscoverNotifiers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS DiscoverNotifiers ${INSTALL_TARGETS_DEFAULT_ARGS})

add_library(Discover::Notifiers ALIAS DiscoverNotifiers)

