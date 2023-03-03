add_executable(plasma-discover-exporter main.cpp DiscoverExporter.cpp DiscoverExporter.h)

target_link_libraries(plasma-discover-exporter Discover::Common KF6::CoreAddons KF6::I18n)
