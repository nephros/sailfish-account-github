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

    property bool _started
    function _valueFromServiceConfig(config, key) {
        return config && config[key] ? config[key].toString() : ""
    }
    function _startUpdate() {
        if (_started || initialPage.status !== PageStatus.Active || account.status !== Account.Initialized) {
            return
        }

        var config = account.configurationValues("github-notifications")
        var apiHost = _valueFromServiceConfig(config, "api/Host")
        var oauthHost = _valueFromServiceConfig(config, "auth/oauth2/web_server/Host")
        var clientId = _valueFromServiceConfig(config, "auth/oauth2/web_server/ClientId")
        var clientSecret = _valueFromServiceConfig(config, "auth/oauth2/web_server/ClientSecret")
        if (clientId.length === 0 || clientSecret.length === 0) {
            //% "Missing GitHub OAuth client credentials"
            credentialsUpdateError(qsTrId("settings-accounts-github-la-missing_client_credentials"))
            return
        }
        var authPath = _valueFromServiceConfig(config, "auth/oauth2/web_server/AuthPath")
        var tokenPath = _valueFromServiceConfig(config, "auth/oauth2/web_server/TokenPath")
        var responseType = _valueFromServiceConfig(config, "auth/oauth2/web_server/ResponseType")
        var oauthScope = _valueFromServiceConfig(config, "auth/oauth2/web_server/Scope")
        var callbackUri = _valueFromServiceConfig(config, "auth/oauth2/web_server/RedirectUri")

        _started = true

        var sessionData = {
            "ClientId": clientId,
            "ClientSecret": clientSecret,
            "Host": oauthHost,
            "AuthPath": authPath,
            "TokenPath": tokenPath,
            "ResponseType": responseType,
            "Scope": oauthScope,
            "RedirectUri": callbackUri
        }
        initialPage.prepareAccountCredentialsUpdate(account, root.accountProvider, "github-notifications", sessionData)
    }

    Account {
        id: account
        identifier: root.accountId

        onStatusChanged: {
            root._startUpdate()
        }
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
