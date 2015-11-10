find_package(Qca-qt5 REQUIRED)

add_library(MuonQOAuth STATIC src/interface.cpp)
target_link_libraries(MuonQOAuth PUBLIC Qt5::Network qca-qt5)
add_library(Muon::QOAuth ALIAS MuonQOAuth)
