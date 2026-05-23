# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025,2026 Peter G. <sailfish@nephros.org>
#
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = subdirs
SUBDIRS += \
    settings \
    buteo-plugins \
    icons

#    common
#    eventsview-plugins

# buteo-plugins.depends = common
# transferengine-plugins.depends = common
# eventsview-plugins.depends = common

OTHER_FILES += rpm/sailfish-account-github.spec
