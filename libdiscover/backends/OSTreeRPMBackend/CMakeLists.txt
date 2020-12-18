set_source_files_properties(org.projectatomic.rpmostree1.xml PROPERTIES
   INCLUDE dbustypes.h
)
qt5_add_dbus_interface(ostree1_SRCS org.projectatomic.rpmostree1.xml rpmostree1)

add_library(ostreerpm-backend MODULE OSTreeRPMBackend.cpp ${ostree1_SRCS})
target_link_libraries(ostreerpm-backend PRIVATE Discover::Common)

install(TARGETS ostreerpm-backend DESTINATION ${PLUGIN_INSTALL_DIR}/discover)
