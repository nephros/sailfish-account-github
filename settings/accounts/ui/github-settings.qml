// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

AccountSettingsAgent {
    id: root

    property string accountSubtitle: {
        var description = account.configurationValues("")["description"]
        var detail = description ? description.toString().trim() : ""
        if (detail.length > 0) {
            return detail
        }
        /*
        var apiHost = account.configurationValues("")["api/Host"]
        var host = apiHost ? apiHost.toString().trim() : ""
        host = host.replace(/^https?:\/\//i, "")
        var pathSeparator = host.indexOf("/")
        if (pathSeparator !== -1) {
            host = host.substring(0, pathSeparator)
        }
        if (host.length > 0) {
            return host
        }
        */
        var displayName = account.displayName ? account.displayName.toString().trim() : ""
        if (displayName.length > 0) {
            return displayName
        }
        //return host
        return "github.com"
    }

    Account {
        id: account
        identifier: root.accountId
    }

    initialPage: Page {
        onPageContainerChanged: {
            if (pageContainer == null && !credentialsUpdater.running) {
                root.delayDeletion = true
                settingsDisplay.saveAccount()
            }
        }

        Component.onDestruction: {
            if (status == PageStatus.Active) {
                settingsDisplay.saveAccount(true)
            }
        }

        AccountCredentialsUpdater {
            id: credentialsUpdater
        }

        SilicaFlickable {
            anchors.fill: parent
            contentHeight: header.height + settingsDisplay.height + Theme.paddingLarge

            StandardAccountSettingsPullDownMenu {
                visible: settingsDisplay.accountValid
                allowSync: true
                onCredentialsUpdateRequested: {
                    credentialsUpdater.replaceWithCredentialsUpdatePage(root.accountId)
                }
                onAccountDeletionRequested: {
                    root.accountDeletionRequested()
                    pageStack.pop()
                }
                onSyncRequested: {
                    settingsDisplay.saveAccountAndSync()
                }
            }

            PageHeader {
                id: header

                title: root.accountsHeaderText
                description: root.accountSubtitle
            }

            GitHubSettingsDisplay {
                id: settingsDisplay

                anchors.top: header.bottom
                accountManager: root.accountManager
                accountProvider: root.accountProvider
                accountId: root.accountId

                onAccountSaveCompleted: {
                    root.delayDeletion = false
                }
            }

            VerticalScrollDecorator {}
        }
    }
}
