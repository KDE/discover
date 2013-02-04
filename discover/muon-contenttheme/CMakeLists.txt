
install(FILES metadata.desktop DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/muon-contenttheme/)

FILE(GLOB widgets widgets/*.svgz)
install( FILES ${widgets} DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/muon-contenttheme/widgets/ )


