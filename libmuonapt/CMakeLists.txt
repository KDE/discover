set(muonapt_SRCS
    ChangesDialog.cpp
    MuonStrings.cpp
    QAptActions.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
)

find_package(KF5 REQUIRED KDELibs4Support)


add_library(MuonApt ${muonapt_SRCS})
target_link_libraries(MuonApt Qt5::Core
     KF5::Solid KF5::KIOWidgets KF5::XmlGui KF5::KDELibs4Support
     QApt::Main muonprivate
)

target_include_directories(MuonApt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(muon_repo_helper AddRepositoryHelper.cpp)
target_link_libraries(muon_repo_helper Qt5::Core KF5::Auth)

install(TARGETS muon_repo_helper DESTINATION ${LIBEXEC_INSTALL_DIR})
kauth_install_actions(org.kde.muon.repo policies/org.kde.muon.repo.action)
kauth_install_helper_files(muon_repo_helper org.kde.muon.repo root)

install(TARGETS MuonApt ${INSTALL_TARGETS_DEFAULT_ARGS})
