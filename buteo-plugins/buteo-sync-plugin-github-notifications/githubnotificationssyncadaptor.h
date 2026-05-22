/*
 * SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-FileCopyrightText: 2024-2026 Peter G. <sailfish@nephros.org>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GITHUBNOTIFICATIONSSYNCADAPTOR_H
#define GITHUBNOTIFICATIONSSYNCADAPTOR_H

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
    struct PendingNotification {
        QString notificationId;
        QString summary;
        QString body;
        QString link;
        QDateTime timestamp;
    };

    void requestNotifications(int accountId, const QString &accessToken,
                              const QString &until = QString(),
                              const QString &pagingToken = QString());

    static int compareNotificationIds(const QString &left, const QString &right);
    QString loadLastFetchedId(int accountId) const;
    void saveLastFetchedId(int accountId, const QString &lastFetchedId);
    void requestMarkRead(int accountId, const QString &accessToken, const QString &lastReadId);
    void publishSystemNotification(int accountId, const PendingNotification &notificationData);
    Notification *createNotification(int accountId, const QString &notificationId);
    Notification *findNotification(int accountId, const QString &notificationId);
    void closeAccountNotifications(int accountId, const QSet<QString> &keepNotificationIds = QSet<QString>());
    static QString notificationObjectKey(int accountId, const QString &notificationId);
    void maybeMarkAccountNotificationsRead(int accountId,
                                           const QString &accessToken,
                                           Notification *ignoredNotification = 0);
    void markReadFromNotification(Notification *notification);
    void removeCachedNotification(Notification *notification);

    QDateTime lastSuccessfulSyncTime(int accountId);
    void setLastSuccessfulSyncTime(int accountId);

private Q_SLOTS:
    void finishedNotificationsHandler();
    void notificationClosedWithReason(uint reason);

private:
    struct NotificationData {
        NotificationData() : accountId(0) {}
        NotificationData(int accountId, const QJsonObject &notification)
            : accountId(accountId), notification(notification) {}
        int accountId;
        QJsonObject notification;
    };
    QHash<int, QString> m_lastMarkedReadIds;
    QHash<QString, Notification *> m_notificationObjects;
    GithubNotificationsDatabase m_db;
};

#endif // GITHUBNOTIFICATIONSSYNCADAPTOR_H
