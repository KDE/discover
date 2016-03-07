include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

add_executable(plasma-discover-exporter main.cpp MuonExporter.cpp)
set_target_properties(plasma-discover-exporter PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover-exporter Discover::Common KF5::CoreAddons KF5::I18n)

if(MUON_INSTALL_EXPORTER)
    install(TARGETS plasma-discover-exporter ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()
