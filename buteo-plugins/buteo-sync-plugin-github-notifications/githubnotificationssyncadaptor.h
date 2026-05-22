/*
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-FileCopyrightText: 2024-2026 Peter G. <sailfish@nephros.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GITHUBNOTIFICATIONSYNCADAPTOR_H
#define GITHUBNOTIFICATIONSYNCADAPTOR_H

#include "githubdatatypesyncadaptor.h"
#include <githubnotificationsdatabase.h>


class Notification;

class GithubNotificationsSyncAdaptor : public GithubNotificationsDataTypeSyncAdaptor
{
    Q_OBJECT

public:
    GithubNotificationsSyncAdaptor(QObject *parent);
    ~GithubNotificationsSyncAdaptor();

    QString syncServiceName() const;

protected:
    void purgeDataForOldAccount(int oldId, SocialNetworkSyncAdaptor::PurgeMode mode) override;
    void beginSync(int accountId, const QString &accessToken) override;
    void finalize(int accountId) override;

private:
    void requestNotifications(int accountId, const QString &accessToken,
                              const QString &until = QString(),
                              const QString &pagingToken = QString());
    QDateTime lastSuccessfulSyncTime(int accountId);
    void setLastSuccessfulSyncTime(int accountId);

private Q_SLOTS:
    void finishedNotificationsHandler();

private:
    struct NotificationData {
        NotificationData() : accountId(0) {}
        NotificationData(int accountId, const QJsonObject &notification)
            : accountId(accountId), notification(notification) {}
        int accountId;
        QJsonObject notification;
    };
    GithubNotificationsDatabase m_db;
};

#endif // GITHUBNOTIFICATIONSYNCADAPTOR_H
