add_subdirectory(tests)
add_definitions( -DPROJECT_NAME=${PROJECT_NAME} -DPROJECT_VERSION=${PROJECT_VERSION})

set(fwupd-backend_SRCS
    FwupdResource.cpp
    FwupdBackend.cpp
    FwupdReviewsBackend.cpp
    FwupdTransaction.cpp
    FwupdSourcesBackend.cpp
    FwupdUpdater.cpp
)




#find_package(GIO)
find_package(Soup)
find_package(GLib)


MESSAGE( STATUS "FWUPD_INCLUDE_DIRS variable is: " ${LIBFWUPD_INCLUDE_DIRS} )
MESSAGE( STATUS "FWUPD_LIBRARIES variable is: " ${LIBFWUPD_LIBRARIES} )

include_directories(${LIBFWUPD_INCLUDE_DIRS} )



add_library(fwupd-backend MODULE ${fwupd-backend_SRCS})
target_link_libraries(fwupd-backend Qt5::Core Qt5::Widgets KF5::CoreAddons KF5::ConfigCore Discover::Common LIBFWUPD  ${Soup} ${GIO} ${GLib} )



install(TARGETS fwupd-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
install(FILES fwupd-backend-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libdiscover/categories)

add_library(FwupdNotifier MODULE FwupdNotifier.cpp )
target_link_libraries(FwupdNotifier Discover::Notifiers)
set_target_properties(FwupdNotifier PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

install(TARGETS FwupdNotifier DESTINATION ${PLUGIN_INSTALL_DIR}/discover-notifier)


