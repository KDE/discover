include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

add_executable(muon-exporter main.cpp MuonExporter.cpp)

target_link_libraries(muon-exporter MuonCommon KF5::CoreAddons KF5::I18n)

if(MUON_INSTALL_EXPORTER)
    install(TARGETS muon-exporter ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()
