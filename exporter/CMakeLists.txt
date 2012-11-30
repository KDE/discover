include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_CURRENT_BINARY_DIR}/..)

kde4_add_executable(muon-exporter main.cpp MuonExporter.cpp)

target_link_libraries(muon-exporter muonprivate
    ${KDE4_KDEUI_LIBRARY} ${QJSON_LIBRARIES} ${qjson_LIBRARIES})

