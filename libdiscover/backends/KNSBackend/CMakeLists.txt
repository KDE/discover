add_subdirectory(knsintegrate)
add_subdirectory(tests)

function(makekns knsfile iconName category)
    get_filename_component(RESOURCE_NAME "${knsfile}" NAME_WE)
    set(backend "kns${RESOURCE_NAME}-backend")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${knsfile}")
        install(FILES ${knsfile} DESTINATION ${KDE_INSTALL_CONFDIR})
        get_filename_component(knsfile "${knsfile}" NAME)
    endif()

    add_custom_target(${knsfile} ALL
        COMMAND knsintegrate ${knsfile} ${iconName} ${category}
        COMMAND cmake -E copy ${CMAKE_CURRENT_BINARY_DIR}/${backend}-categories.xml ${CMAKE_CURRENT_SOURCE_DIR}/${backend}-categories.xml
        DEPENDS knsintegrate
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${backend}.desktop DESTINATION ${DATA_INSTALL_DIR}/libdiscover/backends)
    install(FILES ${backend}-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)
endfunction()

add_library(kns-backend MODULE KNSBackend.cpp
    KNSResource.cpp
    KNSReviews.cpp)

target_link_libraries(kns-backend Discover::Common KF5::ConfigCore KF5::Attica KF5::NewStuff)

install(TARGETS kns-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)

makekns(comic.knsrc face-smile-big "Plasma Addons/Comics")
makekns(plasmoids.knsrc plasma "Plasma Addons/Plasma Desktop Widgets")
makekns(custom/discover_ktexteditor_codesnippets_core.knsrc kate "Application Addons/Kate Snippets")
