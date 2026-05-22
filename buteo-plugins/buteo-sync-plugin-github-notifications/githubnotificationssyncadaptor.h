/*
 * SPDX-FileCopyrightText: 2026 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 2025,2026 Peter G. <sailfish@nephros.org>
 * SPDX-FileCopyrightText: 2026 2026 Jolla Mobile Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GITHUBNOTIFICATIONSYNCADAPTOR_H
#define GITHUBNOTIFICATIONSYNCADAPTOR_H

#include "githubdatatypesyncadaptor.h"
#include <githubnotificationsdatabase.h>

#include <QList>
#include <QJsonObject>

class Notification;

class GithubNotificationSyncAdaptor : public GithubDataTypeSyncAdaptor
{
    Q_OBJECT

public:
    GithubNotificationSyncAdaptor(QObject *parent);
    ~GithubNotificationSyncAdaptor();

    QString syncServiceName() const;

protected: // implementing GithubDataTypeSyncAdaptor interface
    void purgeDataForOldAccount(int oldId, SocialNetworkSyncAdaptor::PurgeMode mode);
    void beginSync(int accountId, const QString &accessToken);
    void finalize(int accountId);

private:
    void requestNotifications(int accountId, const QString &accessToken,
                              const QString &until = QString(),
                              const QString &pagingToken = QString());
    QDateTime lastSuccessfulSyncTime(int accountId);
    void setLastSuccessfulSyncTime(int accountId);

private Q_SLOTS:
    void finishedHandler();

private:
    //void saveGithubNotificationFromObject(int accountId, const QJsonObject &notif);
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
