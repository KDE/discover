include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_CURRENT_BINARY_DIR}/..)

if(QAPT_FOUND)
    add_definitions(-DQAPT_ENABLED)
endif(QAPT_FOUND)

option(LIBATTICA_ENABLED "Enable KNS backend" ON)
if (LIBATTICA_FOUND AND LIBATTICA_ENABLED)
    add_definitions(-DATTICA_ENABLED)
endif (LIBATTICA_FOUND AND LIBATTICA_ENABLED)

kde4_add_executable(muon-exporter main.cpp MuonExporter.cpp)

target_link_libraries(muon-exporter muonprivate muon-appsbackend muon-knsbackend
    ${KDE4_KDEUI_LIBRARY} ${QJSON_LIBRARIES})

