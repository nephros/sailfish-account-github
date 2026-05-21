# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = subdirs
SUBDIRS += \
    buteo-common \
    buteo-sync-plugin-github-posts \
    buteo-sync-plugin-github-notifications

buteo-sync-plugin-github-posts.depends = buteo-common
buteo-sync-plugin-github-notifications.depends = buteo-common
