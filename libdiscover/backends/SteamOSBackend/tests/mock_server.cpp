
#include "mock_server.h"

QString MockServer::currentVersion() const
{
    return TEST_VERSION;
}

QString MockServer::currentBuildID() const
{
    return TEST_BUILDID;
}

QString MockServer::branch() const
{
    return TEST_BRANCH;
}

VariantMapMap MockServer::CheckForUpdates(const QVariantMap & /* options */, VariantMapMap & /* updates_available_later */)
{
    VariantMapMap result;
    return result;
}