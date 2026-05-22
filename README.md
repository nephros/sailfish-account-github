# sailfish-account-github

`sailfish-account-github` adds GitHub integration to Sailfish OS. It lets
you sign in with a GitHub account, and surface GitHub activity in core Sailfish OS views.

## What It Does

- Adds a GitHub account provider to Sailfish OS Accounts.
- Supports sign-in against GitHub servers with OAuth.
- Registers an application on the selected server during setup.
- Shows GitHub notifications in Events view.

## User Experience

After adding an account, GitHub becomes available through the same system
plumbing used by other Sailfish OS online services:

- account setup and credential refresh happen through Sailfish Accounts
- background updates use Buteo sync plugins
- posts appear in Events view

## Development/Testing

As long as this is not included in the main distribution, to test this you will have to:

- At GitHub, create an OAuth configuration
- note down (but keep secret):
  - Application name
  - Client ID
  - Client Secret
  - Redirect Uri
- on your device, create a user-specific `storedkeys.ini` as described in https://github.com/nephros/sfos-github-integration/issues/3
- if you did not build and install an RPM, add the user to the `account-github` group
- maybe update provider and service .xml files with the correct Redirect URI

While deprecated, some of the hints at https://github.com/nephros/sfos-github-integration may still apply.
## Licensing

This repository contains a mix of `BSD-3-Clause` and
`LGPL-2.1-or-later` source files. REUSE metadata in the tree records the
license for each file.

The `LGPL-2.1-or-later` parts are the shared sync and cache layer.

These files are kept under LGPL because they are adapted from existing
Sailfish OS social sync and social cache code, especially the public
`buteo-sync-plugins-social` and `libsocialcache` codebases.

For details see the README of the `sailfish-account-mastodon` package.

The more GitHub-specific helper, UI, and integration files are BSD-licensed
unless noted otherwise.

## Third-Party Marks

GITHUB®, the GITHUB® logo design, the INVERTOCAT logo design, OCTOCAT®, and the
OCTOCAT® logo design are trademarks of GitHub, Inc., registered in the United
States and other countries.

No adaptation or use of any kind of any of our registered trademarks or
copyrights, is allowed without the express written permission of GitHub, Inc.

For more information regarding the authorized uses of these items, please
[contact GitHub](https://github.com/contact)

<!--
SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>

SPDX-License-Identifier: BSD-3-Clause
-->
