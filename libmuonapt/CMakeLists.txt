set(muonapt_SRCS
    ChangesDialog.cpp
    MuonStrings.cpp
    QAptActions.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
)

add_library(muonapt ${muonapt_SRCS})
target_link_libraries(muonapt Qt5::Core
     ${KDE4_KDEUI_LIBS} KF5::Solid KF5::KIOWidgets
    ${QAPT_LIBRARY} muonprivate
)
install(TARGETS muonapt ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(declarative)
