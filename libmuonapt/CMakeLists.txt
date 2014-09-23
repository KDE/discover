set(muonapt_SRCS
    ChangesDialog.cpp
    MuonStrings.cpp
    QAptActions.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
)

add_library(muonapt ${muonapt_SRCS})
target_link_libraries(muonapt Qt5::Core
     KF5::Solid KF5::KIOWidgets KF5::XmlGui
     ${QAPT_LIBRARY} muonprivate
)

add_executable(muon_repo_helper AddRepositoryHelper.cpp)
target_link_libraries(muon_repo_helper Qt5::Core KF5::Auth)

install(TARGETS muon_repo_helper DESTINATION ${LIBEXEC_INSTALL_DIR})
kauth_install_actions(org.kde.muon.repo policies/org.kde.muon.repo.action)
kauth_install_helper_files(muon_repo_helper org.kde.muon.repo root)

install(TARGETS muonapt ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)
