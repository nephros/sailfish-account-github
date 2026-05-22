// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubnotificationsplugin.h"
#include "githubnotificationssyncadaptor.h"
#include "socialnetworksyncadaptor.h"

#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

namespace {
class AppTranslator : public QTranslator
{
public:
    explicit AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    ~AppTranslator() override
    {
        qApp->removeTranslator(this);
    }
};

void ensureNotificationTranslations()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }

    QCoreApplication *app = QCoreApplication::instance();
    if (!app) {
        return;
    }

    AppTranslator *engineeringEnglish = new AppTranslator(app);
    engineeringEnglish->load(QStringLiteral("lipstick-jolla-home-github-notifications_eng_en"),
                             QStringLiteral("/usr/share/translations"));

    AppTranslator *translator = new AppTranslator(app);
    translator->load(QLocale(),
                     QStringLiteral("lipstick-jolla-home-github-notifications"),
                     QStringLiteral("-"),
                     QStringLiteral("/usr/share/translations"));

    initialized = true;
}
}


GithubNotificationsPlugin::GithubNotificationsPlugin(const QString& pluginName,
                                                     const Buteo::SyncProfile& profile,
                                                     Buteo::PluginCbInterface *callbackInterface)
    : SocialdButeoPlugin(pluginName, profile, callbackInterface,
                         QStringLiteral("github"),
                         SocialNetworkSyncAdaptor::dataTypeName(SocialNetworkSyncAdaptor::Notifications))
{
    ensureNotificationTranslations();
}

GithubNotificationsPlugin::~GithubNotificationsPlugin()
{
}

SocialNetworkSyncAdaptor *GithubNotificationsPlugin::createSocialNetworkSyncAdaptor()
{
    return new GithubNotificationsSyncAdaptor(this);
}

Buteo::ClientPlugin* GithubNotificationsPluginLoader::createClientPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface* cbInterface)
{
    return new GithubNotificationsPlugin(pluginName, profile, cbInterface);
}
