add_executable(plasma-discover-update main.cpp DiscoverUpdate.cpp)

set_target_properties(plasma-discover-update PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)
target_link_libraries(plasma-discover-update Discover::Common KF6::CoreAddons KF6::I18n KF6::DBusAddons)

install(TARGETS plasma-discover-update ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
