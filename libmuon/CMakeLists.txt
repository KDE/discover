set(muon_LIB_SRCS
    MuonMainWindow.cpp
    MuonStrings.cpp
    CommitWidget.cpp
    DownloadWidget.cpp
    StatusWidget.cpp
    DetailsWidget.cpp
    DownloadModel/DownloadModel.cpp
    DownloadModel/DownloadDelegate.cpp
    PackageModel/PackageModel.cpp
    PackageModel/PackageProxyModel.cpp
    PackageModel/PackageView.cpp
    PackageModel/PackageDelegate.cpp
    DetailsTabs/MainTab.cpp
    DetailsTabs/ChangelogTab.cpp
    DetailsTabs/DependsTab.cpp
    DetailsTabs/InstalledFilesTab.cpp
    DetailsTabs/TechnicalDetailsTab.cpp
    DetailsTabs/VersionTab.cpp)

kde4_add_library( libmuon  STATIC ${muon_LIB_SRCS} )
