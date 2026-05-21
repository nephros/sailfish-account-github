/*
 * SPDX-FileCopyrightText: 2026 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
 * SPDX-FileCopyrightText: 2026 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GITHUBNOTIFICATIONSPLUGIN_H
#define GITHUBNOTIFICATIONSPLUGIN_H

#include "socialdbuteoplugin.h"

#include <buteosyncfw5/SyncPluginLoader.h>

class Q_DECL_EXPORT GithubNotificationsPlugin : public SocialdButeoPlugin
{
    Q_OBJECT

public:
    GithubNotificationsPlugin(const QString& pluginName,
                  const Buteo::SyncProfile& profile,
                  Buteo::PluginCbInterface *cbInterface);
    ~GithubNotificationsPlugin();

protected:
    SocialNetworkSyncAdaptor *createSocialNetworkSyncAdaptor();
};


class GithubNotificationsPluginLoader : public Buteo::SyncPluginLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfishos.plugins.sync.GithubNotificationsPluginLoader")
    Q_INTERFACES(Buteo::SyncPluginLoader)

public:
    Buteo::ClientPlugin* createClientPlugin(const QString& pluginName,
                                            const Buteo::SyncProfile& profile,
                                            Buteo::PluginCbInterface* cbInterface) override;
};

#endif // GITHUBNOTIFICATIONSPLUGIN_H
