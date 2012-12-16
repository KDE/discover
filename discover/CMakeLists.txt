add_subdirectory(icons)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

set(muon_discover_SRCS
    main.cpp
    MuonDiscoverMainWindow.cpp
)

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
install(FILES featured.json DESTINATION ${DATA_INSTALL_DIR}/muon-discover)
