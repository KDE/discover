include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_CURRENT_BINARY_DIR}/..)

add_executable(muon-exporter main.cpp MuonExporter.cpp)

target_link_libraries(muon-exporter muonprivate KF5::CoreAddons KF5::I18n)

