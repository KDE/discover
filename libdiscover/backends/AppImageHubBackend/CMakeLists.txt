ecm_qt_declare_logging_category(appimagehub-backend-logging_SRCS
    HEADER libdiscover_backend_appimagehub_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_APPIMAGEHUB_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.appimagehub
    DESCRIPTION "libdiscover (backend - AppImageHub)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-appimagehub-logging STATIC ${appimagehub-backend-logging_SRCS})
set_property(TARGET libdiscover-backend-appimagehub-logging PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(libdiscover-backend-appimagehub-logging PRIVATE Qt::Core)

set(appimagehub-backend_SRCS
    AppImageHubBackend.cpp
    AppImageHubResource.cpp
    AppImageHubTransaction.cpp
)

kcoreaddons_add_plugin(appimagehub-backend SOURCES ${appimagehub-backend_SRCS} INSTALL_NAMESPACE "discover")
target_link_libraries(appimagehub-backend
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Network
        Discover::Common
        KF6::CoreAddons
        libdiscover-backend-appimagehub-logging
)