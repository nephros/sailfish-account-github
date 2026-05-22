// SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
// SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
// SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
//
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

AccountCredentialsAgent {
    id: root

    function _startUpdate() {
        if (initialPage.status !== PageStatus.Active || account.status !== Account.Initialized) {
            return
        }
        var sessionData = {
            "ClientId": keyProvider.storedKey("github", "", "client_id"),
            "ClientSecret": keyProvider.storedKey("github", "", "client_secret")
        }
        initialPage.prepareAccountCredentialsUpdate(account, root.accountProvider, "github-sync", sessionData)
    }

    Account {
        id: account
        identifier: root.accountId

        onStatusChanged: {
            root._startUpdate()
        }
    }

    StoredKeyProvider {
        id: keyProvider
    }

    initialPage: OAuthAccountSetupPage {
        onStatusChanged: {
            root._startUpdate()
        }

        onAccountCredentialsUpdated: {
            root.credentialsUpdated(root.accountId)
            root.goToEndDestination()
        }

        onAccountCredentialsUpdateError: {
            root.credentialsUpdateError(errorMessage)
        }

        onPageContainerChanged: {
            if (pageContainer == null) {
                cancelSignIn()
            }
        }
    }
}
