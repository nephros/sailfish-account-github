// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2023-2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GITHUBNOTIFICATIONSDATABASE_H
#define GITHUBNOTIFICATIONSDATABASE_H

#include "abstractsocialcachedatabase.h"

#include <QtCore/QSharedPointer>
#include <QStringList>
#include <QDateTime>

class GithubNotificationPrivate;
class GithubNotification
{
public:
    typedef QSharedPointer<GithubNotification> Ptr;
    typedef QSharedPointer<const GithubNotification> ConstPtr;

    virtual ~GithubNotification();

    static GithubNotification::Ptr create(int accountId,
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
    int accountId() const;
    int threadId() const;
    QString type() const;
    QString title() const;
    QString from() const;
    QString reason() const;
    bool    unread() const;
    QString repo() const;
    QString avatar() const;
    QString url() const;
    QDateTime updatedTime() const;


protected:
    QScopedPointer<GithubNotificationPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(GithubNotification)
    explicit GithubNotification(int accountId,
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
};


class GithubNotificationsDatabasePrivate;
class GithubNotificationsDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
    Q_PROPERTY(QVariantList accountIdFilter READ accountIdFilter WRITE setAccountIdFilter NOTIFY accountIdFilterChanged)

public:
    explicit GithubNotificationsDatabase();
    ~GithubNotificationsDatabase();

    QVariantList accountIdFilter() const;
    void setAccountIdFilter(const QVariantList &accountIds);

    void addGithubNotification(int accountId,
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

    void removeAllNotifications();
    void removeNotifications(int accountId);
    void removeNotification(const QString &notificationId);
    void removeNotifications(const QStringList &notificationIds);

    void sync();

    QList<GithubNotification::ConstPtr> notifications();

signals:
    void notificationsChanged();
    void accountIdFilterChanged();

protected:
    void readFinished();
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(GithubNotificationsDatabase)
};

#endif // GITHUBNOTIFICATIONSDATABASE_H
