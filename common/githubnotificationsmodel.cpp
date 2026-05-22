// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2023-2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubnotificationsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "githubnotificationsdatabase.h"

class GithubNotificationsModelPrivate : public AbstractSocialCacheModelPrivate
{
public:
    explicit GithubNotificationsModelPrivate(GithubNotificationsModel *q);

    GithubNotificationsDatabase database;

private:
    // note CamelCase here:
    Q_DECLARE_PUBLIC(GitHubNotificationsModel)
};

GithubNotificationsModelPrivate::GithubNotificationsModelPrivate(GithubNotificationsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

GithubNotificationsModel::GithubNotificationsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new GithubNotificationsModelPrivate(this)), parent)
{
    Q_D(GithubNotificationsModel);

    connect(&d->database, SIGNAL(notificationsChanged()), this, SLOT(notificationsChanged()));
    connect(&d->database, SIGNAL(accountIdFilterChanged()), this, SIGNAL(accountIdFilterChanged()));
}

QHash<int, QByteArray> GithubNotificationsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(NotificationId, "threadId");
    roleNames.insert(Type,   "type");
    roleNames.insert(Title,  "title");
    roleNames.insert(From,   "from");
    roleNames.insert(Reason, "reason");
    roleNames.insert(Unread, "unread");
    roleNames.insert(Repo,   "repo");
    roleNames.insert(Avatar, "avatar");
    roleNames.insert(Link,    "url");
    roleNames.insert(TimeStamp, "timestamp");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

QVariantList GithubNotificationsModel::accountIdFilter() const
{
    Q_D(const GithubNotificationsModel);

    return d->database.accountIdFilter();
}

void GithubNotificationsModel::setAccountIdFilter(const QVariantList &accountIds)
{
    Q_D(GithubNotificationsModel);

    d->database.setAccountIdFilter(accountIds);
}

void GithubNotificationsModel::refresh()
{
    notificationsChanged();
}

void GithubNotificationsModel::remove(const QString &notificationId)
{
    Q_D(GithubNotificationsModel);
    for (int i=0; i<count(); i++) {
        if (getField(i, GithubNotificationsModel::NotificationId).toString() == notificationId) {
            d->removeRange(i, 1);
            d->database.removeNotification(notificationId);
            d->database.sync();
            break;
        }
    }
}

void GithubNotificationsModel::clear()
{
    Q_D(GithubNotificationsModel);
    d->clearData();
    d->database.removeAllNotifications();
}

void GithubNotificationsModel::notificationsChanged()
{
    Q_D(GithubNotificationsModel);

    SocialCacheModelData data;
    QList<GithubNotification::ConstPtr> notificationsData = d->database.notifications();
    Q_FOREACH (const GithubNotification::ConstPtr &notification, notificationsData) {
        QMap<int, QVariant> eventMap;

        eventMap.insert(GithubNotificationsModel::NotificationId, notification->threadId());
        eventMap.insert(GithubNotificationsModel::Type, notification->type());
        eventMap.insert(GithubNotificationsModel::Title, notification->title());
        eventMap.insert(GithubNotificationsModel::From, notification->from());
        eventMap.insert(GithubNotificationsModel::Repo, notification->repo());
        eventMap.insert(GithubNotificationsModel::Reason, notification->reason());
        eventMap.insert(GithubNotificationsModel::Unread, notification->unread());
        eventMap.insert(GithubNotificationsModel::Avatar, notification->avatar());
        eventMap.insert(GithubNotificationsModel::Link, notification->url());
        eventMap.insert(GithubNotificationsModel::TimeStamp, notification->updatedTime());

        QVariantList accountsVariant;
        accountsVariant.append(notification->accountId());
        eventMap.insert(GithubNotificationsModel::Accounts, accountsVariant);

        //eventMap.insert(GithubNotificationsModel::ClientId, notification->clientId());

        data.append(eventMap);
    }

    updateData(data);
}
