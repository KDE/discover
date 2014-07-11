include_directories(. ${CMAKE_SOURCE_DIR}/libmuonapt)

add_subdirectory(tests)

set(appsbackend_SRCS
    ApplicationBackend.cpp
    Application.cpp
    ApplicationUpdates.cpp
    ReviewsBackend.cpp #TODO: rename to AptReviewsBackend
    UbuntuLoginBackend.cpp
)

qt5_add_dbus_interface(appsbackend_SRCS ubuntu_sso_dbus_interface.xml ubuntu_sso OPTIONS -i "LoginMetaTypes.h")

add_library(muon-appsbackend MODULE ${appsbackend_SRCS})
target_link_libraries(muon-appsbackend Qt5::Widgets Qt5::DBus Qt5::Concurrent
     KF5::Archive KF5::KIOWidgets DebconfKDE::Main
     ${QTOAUTH_LIBRARY} ${QAPT_LIBRARY} muonprivate muonapt
)
target_include_directories(muon-appsbackend PRIVATE ${QCA_INCLUDES}) #qtoauth should be dragging it, but it doesn't

install(TARGETS muon-appsbackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-applications-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES apps-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_library(kded_muonapplicationnotifier MODULE ApplicationNotifier.cpp)
target_link_libraries(kded_muonapplicationnotifier ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} muonprivate)
target_include_directories(kded_muonapplicationnotifier PRIVATE ${CMAKE_SOURCE_DIR}/kded)

install(TARGETS kded_muonapplicationnotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muonapplicationnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
install(FILES muonapplicationnotifier.notifyrc DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
install(FILES distupgradeevent/releasechecker DESTINATION ${DATA_INSTALL_DIR}/muonapplicationnotifier)
