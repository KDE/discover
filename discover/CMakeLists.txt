add_subdirectory(tests)
add_subdirectory(icons)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libmuon ${CMAKE_CURRENT_BINARY_DIR}/..)

set(muon_discover_SRCS
    main.cpp
    MuonDiscoverMainWindow.cpp
    ApplicationProxyModelHelper.cpp
    ReviewsModel.cpp
    ApplicationAddonsModel.cpp
    ScreenshotsModel.cpp
)

if(QAPT_FOUND)
    set(muon_discover_SRCS ${muon_discover_SRCS}
        OriginsBackend.cpp
    )
    add_definitions(-DQAPT_ENABLED)
endif(QAPT_FOUND)

option(LIBATTICA_ENABLED "Enable KNS backend" ON)
if (LIBATTICA_FOUND AND LIBATTICA_ENABLED)
    add_definitions(-DATTICA_ENABLED)
endif (LIBATTICA_FOUND AND LIBATTICA_ENABLED)

qt4_add_resources(muon_discover_SRCS resources.qrc)

kde4_add_executable(muon-discover ${muon_discover_SRCS})

target_link_libraries(muon-discover muonprivate
                                     ${KDE4_KIO_LIBS}
                                     ${KDE4_PLASMA_LIBS}
                                     kdeclarative
                                     ${QAPT_LIBRARY}
                                     ${QT_QTOPENGL_LIBRARY}
                                     ${QT_QTDECLARATIVE_LIBRARY}
)

install(TARGETS muon-discover ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS muon-discover.desktop muon-discover-category.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR} )
