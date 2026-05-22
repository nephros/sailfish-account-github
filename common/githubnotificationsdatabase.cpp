// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2023-2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "githubnotificationsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlError>
#include <QtCore/QtDebug>

static const char *DB_NAME = "githubNotifications.db";
static const int VERSION = 0;

struct GithubNotificationPrivate
{
    explicit GithubNotificationPrivate(int accountId,
                                   const int &threadId,
                                   const QString &type,
                                   const QString &title,
                                   const QString &from,
                                   const QString &reason,
                                   const bool    &unread,
                                   const QString &repo,
                                   const QString &avatar,
                                   const QString &url,
                                   const QDateTime &updatedTime);

    int m_threadId;
    int m_accountId;
    QString m_type;
    QString m_title;
    QString m_from;
    QString m_reason;
    bool    m_unread;
    QString m_repo;
    QString m_avatar;
    QString m_url;
    QDateTime m_updatedTime;
};

GithubNotificationPrivate::GithubNotificationPrivate(int accountId,
                                             const int &threadId,
                                             const QString &type,
                                             const QString &title,
                                             const QString &from,
                                             const QString &reason,
                                             const bool    &unread,
                                             const QString &repo,
                                             const QString &avatar,
                                             const QString &url,
                                             const QDateTime &updatedTime)
    : m_accountId(accountId)
    , m_threadId(threadId)
    , m_type(type)
    , m_title(title)
    , m_from(from)
    , m_reason(reason)
    , m_unread(unread)
    , m_repo(repo)
    , m_avatar(avatar)
    , m_url(url)
    , m_updatedTime(updatedTime)
{
}

GithubNotification::GithubNotification(int accountId,
                               const int &threadId,
                               const QString &type,
                               const QString &title,
                               const QString &from,
                               const QString &reason,
                               const bool    &unread,
                               const QString &repo,
                               const QString &avatar,
                               const QString &url,
                               const QDateTime &updatedTime)
    : d_ptr(new GithubNotificationPrivate(accountId, threadId, type, title, from, reason, unread, repo, avatar, url, updatedTime))
{
}

GithubNotification::Ptr GithubNotification::create(int accountId,
                                           const int &threadId,
                                           const QString &type,
                                           const QString &title,
                                           const QString &from,
                                           const QString &reason,
                                           const bool    &unread,
                                           const QString &repo,
                                           const QString &avatar,
                                           const QString &url,
                                           const QDateTime &updatedTime)
{
    return GithubNotification::Ptr(new GithubNotification(accountId, threadId, type, title, from, reason, unread, repo, avatar, url, updatedTime));
}

GithubNotification::~GithubNotification()
{
}

int GithubNotification::threadId() const
{
    Q_D(const GithubNotification);
    return d->m_threadId;
}

QString GithubNotification::title() const
{
    Q_D(const GithubNotification);
    return d->m_title;
}

QString GithubNotification::type() const
{
    Q_D(const GithubNotification);
    return d->m_type;
}

QString GithubNotification::from() const
{
    Q_D(const GithubNotification);
    return d->m_from;
}

QString GithubNotification::reason() const
{
    Q_D(const GithubNotification);
    return d->m_reason;
}

bool GithubNotification::unread() const
{
    Q_D(const GithubNotification);
    return d->m_unread;
}

QString GithubNotification::repo() const
{
    Q_D(const GithubNotification);
    return d->m_repo;
}

QString GithubNotification::avatar() const
{
    Q_D(const GithubNotification);
    return d->m_avatar;
}

QString GithubNotification::url() const
{
    Q_D(const GithubNotification);
    return d->m_url;
}

QDateTime GithubNotification::updatedTime() const
{
    Q_D(const GithubNotification);
    return d->m_updatedTime;
}

int GithubNotification::accountId() const
{
    Q_D(const GithubNotification);
    return d->m_accountId;
}


class GithubNotificationsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit GithubNotificationsDatabasePrivate(GithubNotificationsDatabase *q);

    QMap<int, QList<GithubNotification::ConstPtr> > insertNotifications;
    QList<int> removeNotificationsFromAccounts;
    QStringList removeNotifications;

    QVariantList accountIdFilter;

    struct {
        QMap<int, QList<GithubNotification::ConstPtr> > insertNotifications;
        QList<int> removeNotificationsFromAccounts;
        QStringList removeNotifications;
    } queue;
};

GithubNotificationsDatabasePrivate::GithubNotificationsDatabasePrivate(GithubNotificationsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Github),
            SocialSyncInterface::dataType(SocialSyncInterface::Notifications),
            QLatin1String(DB_NAME),
            VERSION)
{
}

GithubNotificationsDatabase::GithubNotificationsDatabase()
    : AbstractSocialCacheDatabase(*(new GithubNotificationsDatabasePrivate(this)))
{
}

GithubNotificationsDatabase::~GithubNotificationsDatabase()
{
    wait();
}

QVariantList GithubNotificationsDatabase::accountIdFilter() const
{
    Q_D(const GithubNotificationsDatabase);

    return d->accountIdFilter;
}

void GithubNotificationsDatabase::setAccountIdFilter(const QVariantList &accountIds)
{
    Q_D(GithubNotificationsDatabase);

    if (d->accountIdFilter != accountIds) {
        d->accountIdFilter = accountIds;
        emit accountIdFilterChanged();
    }
}

void GithubNotificationsDatabase::addGithubNotification(int accountId,
                                                const int &threadId,
                                                const QString &type,
                                                const QString &title,
                                                const QString &from,
                                                const QString &reason,
                                                const bool    &unread,
                                                const QString &repo,
                                                const QString &avatar,
                                                const QString &url,
                                                const QDateTime &updatedTime)
{
    Q_D(GithubNotificationsDatabase);
    d->insertNotifications[accountId].append(GithubNotification::create(accountId, threadId, type, title, from, reason, unread, repo, avatar, url, updatedTime));
}

void GithubNotificationsDatabase::removeAllNotifications()
{
   //FIXME: this is in the qml plugin
   qWarning() << Q_FUNC_INFO << "Not implemented";
}

void GithubNotificationsDatabase::removeNotifications(int accountId)
{
    Q_D(GithubNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removeNotificationsFromAccounts.contains(accountId)) {
        d->queue.removeNotificationsFromAccounts.append(accountId);
    }
    d->queue.insertNotifications.remove(accountId);
}

void GithubNotificationsDatabase::removeNotification(const QString &notificationId)
{
    Q_D(GithubNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removeNotifications.contains(notificationId)) {
        d->queue.removeNotifications.append(notificationId);
    }
}

void GithubNotificationsDatabase::removeNotifications(const QStringList &notificationIds)
{
    Q_D(GithubNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    Q_FOREACH(const QString notifId, notificationIds) {
        removeNotification(notifId);
    }
}

void GithubNotificationsDatabase::sync()
{
    Q_D(GithubNotificationsDatabase);

    {
        QMutexLocker locker(&d->mutex);
        Q_FOREACH(int accountId, d->insertNotifications.keys()) {
            d->queue.insertNotifications.insert(accountId, d->insertNotifications.take(accountId));
        }
        while (d->removeNotificationsFromAccounts.count()) {
            d->queue.removeNotificationsFromAccounts.append(d->removeNotificationsFromAccounts.takeFirst());
        }
        while (d->removeNotifications.count()) {
            d->queue.removeNotifications.append(d->removeNotifications.takeFirst());
        }
    }

    executeWrite();
}

QList<GithubNotification::ConstPtr> GithubNotificationsDatabase::notifications()
{
    QList<GithubNotification::ConstPtr> data;

    QSqlQuery query;
    query = prepare(QStringLiteral(
                "SELECT threadId, accountId, typeStr, titleStr, fromStr, reasonStr, unread, repoStr, avatarUrl, url, updatedTime " \
                "FROM notifications ORDER BY updatedTime DESC"));

    if (!query.exec()) {
        qWarning() << "Failed to query events:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(GithubNotification::create(
                                           query.value(0).toInt(),         // threadId
                                           query.value(1).toInt(),         // accountId
                                           query.value(2).toString(),                       // type
                                           query.value(3).toString(),                       // title
                                           query.value(4).toString(),                       // from
                                           query.value(5).toString(),                       // reason
                                           query.value(6).toBool(),                         // unread
                                           query.value(7).toString(),                       // repo
                                           query.value(8).toString(),                       // avatar
                                           query.value(9).toString(),                       // url
                                           QDateTime::fromTime_t(query.value(10).toInt())));// updatedTime
    }

    return data;
}

void GithubNotificationsDatabase::readFinished()
{
    emit notificationsChanged();
}

bool GithubNotificationsDatabase::write()
{
    Q_D(GithubNotificationsDatabase);

    QMutexLocker locker(&d->mutex);

    const QMap<int, QList<GithubNotification::ConstPtr> > insertNotifications = d->queue.insertNotifications;
    const QList<int> removeNotificationsFromAccounts = d->queue.removeNotificationsFromAccounts;
    QStringList removeNotifications = d->queue.removeNotifications;

    d->queue.insertNotifications.clear();
    d->queue.removeNotificationsFromAccounts.clear();
    d->queue.removeNotifications.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!removeNotificationsFromAccounts.isEmpty()) {
        QVariantList accountIds;

        Q_FOREACH (const int accountId, removeNotificationsFromAccounts) {
            accountIds.append(accountId);
        }

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeNotifications.isEmpty()) {
        QVariantList notifIds;

        Q_FOREACH (const QString notifId, removeNotifications) {
            notifIds.append(notifId.toInt());
        }

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE threadId = :threadId"));
        query.bindValue(QStringLiteral(":threadId"), notifIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertNotifications.isEmpty()) {
        QVariantList threadIds;
        QVariantList accountIds;
        QVariantList types;
        QVariantList titles;
        QVariantList froms;
        QVariantList reasons;
        QVariantList unreads;
        QVariantList repos;
        QVariantList avatars;
        QVariantList urls;
        QVariantList updatedTimes;

        Q_FOREACH (const QList<GithubNotification::ConstPtr> &notifications, insertNotifications) {
            Q_FOREACH (const GithubNotification::ConstPtr &notification, notifications) {
                accountIds.append(notification->accountId());
                threadIds.append(notification->threadId());
                types.append(notification->type());
                titles.append(notification->title());
                froms.append(notification->from());
                reasons.append(notification->reason());
                unreads.append(notification->unread());
                repos.append(notification->repo());
                avatars.append(notification->avatar());
                urls.append(notification->url());
                updatedTimes.append(notification->updatedTime().toTime_t());
            }
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO notifications ("
                    "threadId, accountId, typeStr, titleStr, fromStr, reasonStr, unread, repoStr, avatarUrl, url, updatedTime) "
                    "VALUES("
                    ":threadId, :accountId, :typeStr, :titleStr, :fromStr, :reasonStr, :unread, :repoStr, :avatarUrl, :url, :updatedTime)"));
        query.bindValue(QStringLiteral(":threadId"), threadIds);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":typeStr"), types);
        query.bindValue(QStringLiteral(":titleStr"), titles);
        query.bindValue(QStringLiteral(":fromStr"), froms);
        query.bindValue(QStringLiteral(":reasonStr"), reasons);
        query.bindValue(QStringLiteral(":unread"), unreads);
        query.bindValue(QStringLiteral(":repoStr"), repos);
        query.bindValue(QStringLiteral(":avatarUrl"), avatars);
        query.bindValue(QStringLiteral(":url"), urls);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);

        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool GithubNotificationsDatabase::createTables(QSqlDatabase database) const
{
        QSqlQuery query(database);

        query.prepare("CREATE TABLE IF NOT EXISTS notifications ("\
                  "threadId INTEGER UNIQUE PRIMARY KEY,"\
                  "accountId INTEGER,"\
                  "typeStr TEXT,"\
                  "titleStr TEXT,"\
                  "fromStr TEXT,"\
                  "reasonStr TEXT,"\
                  "unread BOOLEAN CHECK (unread IN (0, 1)),"\
                  "repoStr TEXT,"\
                  "avatarUrl TEXT,"\
                  "url TEXT,"\
                  "updatedTime INTEGER)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create notifications table: " << query.lastError().text();
        return false;
    }

    qInfo() << "Created notifications table.";
    return true;
}

bool GithubNotificationsDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS notifications"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete notifications table: " << query.lastError().text();
        return false;
    }

    return true;
}
