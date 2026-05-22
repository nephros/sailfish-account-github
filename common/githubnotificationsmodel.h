// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2023-2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef GITHUBNOTIFICATIONSMODEL_H
#define GITHUBNOTIFICATIONSMODEL_H
#include <QtCore/QAbstractListModel>
#include <QHash>

class GithubNotificationsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList accountIdFilter READ accountIdFilter WRITE setAccountIdFilter NOTIFY accountIdFilterChanged)

    Q_ENUMS(GithubNotificationsRole)
public:
    enum GithubNotificationsRole {
        NotificationId = 0,
        Type,
        Title,
        From,
        Reason,
        Unread,
        Repo,
        Avatar,
        Link,
        TimeStamp,
        Accounts,
    };
    explicit GithubNotificationsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList accountIdFilter() const;
    void setAccountIdFilter(const QVariantList &accountIds);

    void refresh();

    Q_INVOKABLE void remove(const QString &notificationId);
    Q_INVOKABLE void clear();

signals:
    void accountIdFilterChanged();
    void countChanged();

private Q_SLOTS:
    void notificationsChanged();

private:
    Q_DECLARE_PRIVATE(GithubNotificationsModel)
    QVariantList m_accountIdFilterStub;
    typedef QMap<int, QVariant> RowData;
    QList<RowData> m_data;
};

#endif // GITHUBNOTIFICATIONSMODEL_H
