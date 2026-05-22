/****************************************************************************
 **
 ** Copyright (C) 2014 - 2015 Jolla Ltd.
 ** Copyright (C) 2020 Open Mobile Platform LLC.
 ** Copyright (C) 2023 Peter G.
 **
 ****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.social 1.0
import org.nemomobile.socialcache 1.0
import "shared"

SocialMediaAccountDelegate {
    id: delegateItem

    headerText: "GitHub Notifications"
    headerIcon: ( Theme.colorScheme === Theme.LightOnDark ) ? "image://theme/github-mark-white" : "image://theme/github-mark"

    services: [ "Notifications"]
    socialNetwork: SocialSync.Github
    dataType: SocialSync.Notifications

    model: GithubNotificationsModel {}

    delegate: GithubFeedItem {
        accountId: model.accounts[0]
        userRemovable: true
        animateRemoval: defaultAnimateRemoval || delegateItem.removeAllInProgress

        onRemoveRequested: {
            delegateItem.model.remove(model.threadId)
        }

        onTriggered: {
            console.log("GHN triggered! Opening %1".arg(model.link))
            Qt.openUrlExternally(model.link)
        }

        Component.onCompleted: {
            refreshTimeCount = Qt.binding(function() { return delegateItem.refreshTimeCount })
            connectedToNetwork = Qt.binding(function() { return delegateItem.connectedToNetwork })
            eventsColumnMaxWidth = Qt.binding(function() { return delegateItem.eventsColumnMaxWidth })
        }
    }

    //% "Show more on GitHub"
    expandedLabel: qsTrId("lipstick-jolla-home-la-show-more-on-github")
    userRemovable: true

    onHeaderClicked: Qt.openUrlExternally("https://github.com/notifications")
    onExpandedClicked: Qt.openUrlExternally("https://github.com/notifications")

    onViewVisibleChanged: {
        if (viewVisible) {
            delegateItem.resetHasSyncableAccounts()
            delegateItem.model.refresh()
        }
    }
}
