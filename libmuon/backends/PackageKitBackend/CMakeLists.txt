pkg_check_modules(QPACKAGEKIT2 REQUIRED packagekit-qt2>=0.6.17)
include_directories(${QPACKAGEKIT2_INCLUDEDIR})

set(pkbackend_SRCS
    PackageKitBackend.cpp
    PackageKitResource.cpp
    AppPackageKitResource.cpp
    AppstreamUtils.cpp
    PKTransaction.cpp
    PackageKitUpdater.cpp
)

set(packagekitnotifier_SRCS
    PackageKitNotifier.cpp
)

kde4_add_plugin(muon-pkbackend SHARED ${pkbackend_SRCS})
target_link_libraries(muon-pkbackend muonprivate ${QT_QTCORE_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QPACKAGEKIT2_LIBRARIES})

install(TARGETS muon-pkbackend DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES muon-packagekit-backend.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES packagekit-categories.xml DESTINATION ${DATA_INSTALL_DIR}/libmuon/categories)

add_subdirectory(tests)

#notifier
kde4_add_plugin(kded_muonpackagekitnotifier ${packagekitnotifier_SRCS})

target_link_libraries(kded_muonpackagekitnotifier ${QPACKAGEKIT2_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} muonprivate)

install(TARGETS kded_muonpackagekitnotifier DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES muonpackagekitnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded)
