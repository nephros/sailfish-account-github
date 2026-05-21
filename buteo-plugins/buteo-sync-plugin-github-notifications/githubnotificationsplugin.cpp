// SPDX-FileCopyrightText: 2026 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
// SPDX-FileCopyrightText: 2026 2026 Jolla Mobile Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include "githubnotificationsplugin.h"
#include "githubnotificationssyncadaptor.h"
#include "socialnetworksyncadaptor.h"

GithubNotificationsPlugin::GithubNotificationsPlugin(const QString& pluginName,
                             const Buteo::SyncProfile& profile,
                             Buteo::PluginCbInterface *callbackInterface)
    : SocialdButeoPlugin(pluginName, profile, callbackInterface,
                         QStringLiteral("github"),
                         SocialNetworkSyncAdaptor::dataTypeName(SocialNetworkSyncAdaptor::Notifications))
{
}

GithubNotificationsPlugin::~GithubNotificationsPlugin()
{
}

SocialNetworkSyncAdaptor *GithubNotificationsPlugin::createSocialNetworkSyncAdaptor()
{
    return new GithubNotificationSyncAdaptor(this);
}

Buteo::ClientPlugin* GithubNotificationsPluginLoader::createClientPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface* cbInterface)
{
    return new GithubNotificationsPlugin(pluginName, profile, cbInterface);
}
