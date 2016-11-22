add_executable(runservice main.cpp)
target_link_libraries(runservice KF5::Service KF5::KIOCore)
install(TARGETS runservice DESTINATION ${KDE_INSTALL_LIBEXECDIR_KF5}/discover)
