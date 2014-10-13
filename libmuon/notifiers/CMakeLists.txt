add_library(MuonNotifiers BackendNotifierModule.cpp)
target_link_libraries(MuonNotifiers
    PUBLIC
        Qt5::Core
)
target_include_directories(MuonNotifiers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS MuonNotifiers ${INSTALL_TARGETS_DEFAULT_ARGS})
