set(muonapt_SRCS
    ChangesDialog.cpp
    MuonStrings.cpp
    QAptActions.cpp
    HistoryView/HistoryProxyModel.cpp
    HistoryView/HistoryView.cpp
)

kde4_add_library(muonapt SHARED ${muonapt_SRCS})
target_link_libraries(muonapt ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS}
    ${QAPT_LIBRARY} muonprivate
)
install(TARGETS muonapt ${INSTALL_TARGETS_DEFAULT_ARGS})
