set(muonapt_SRCS
    ChangesDialog.cpp
    MuonStrings.cpp
    QAptActions.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
)


kde4_add_library(muonapt SHARED ${muonapt_SRCS})
kde4_install_auth_actions(org.kde.muon.repo policies/org.kde.muon.repo.action)

kde4_add_executable(muon_repo_helper AddRepositoryHelper.cpp)
target_link_libraries(muon_repo_helper ${KDE4_KDECORE_LIBS})

install(TARGETS muon_repo_helper DESTINATION ${LIBEXEC_INSTALL_DIR})

kde4_install_auth_helper_files(muon_repo_helper org.kde.muon.repo root)

target_link_libraries(muonapt ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS}
    ${QAPT_LIBRARY} muonprivate
)
install(TARGETS muonapt ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)