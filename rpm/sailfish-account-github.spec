# SPDX-FileCopyrightText: 2019 - 2023 Jolla Ltd.
# SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
# SPDX-FileCopyrightText: 2025-2026 Peter G. <sailfish@nephros.org>

#
# SPDX-License-Identifier: BSD-3-Clause

Name: sailfish-account-github
License: BSD-3-Clause AND LGPL-2.1-or-later
Version: 1.0.0
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: SailfishOS account plugin for GitHub
BuildRequires: qt5-qmake
BuildRequires: qt5-qttools-linguist
BuildRequires: sailfish-svg2png
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(mlite5)
BuildRequires: pkgconfig(buteosyncfw5) >= 0.10.0
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(socialcache)
BuildRequires: pkgconfig(libsailfishkeyprovider)
BuildRequires: pkgconfig(sailfishaccounts)
BuildRequires: pkgconfig(nemonotifications-qt5)
Requires: jolla-settings-accounts-extensions-onlinesync
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires: lipstick-jolla-home-qt5-components >= 1.2.50
#Requires: eventsview-extensions
Requires: sailfishsilica-qt5 >= 1.1.108
Requires(post): %{_libexecdir}/manage-groups
Requires(postun): %{_libexecdir}/manage-groups

%description
%{summary}.

%package -n sailfish-account-github-ts-devel
Summary: Translation source files for sailfish-account-github
Requires: %{name} = %{version}-%{release}

%description -n sailfish-account-github-ts-devel
Translation source files for sailfish-account-github components.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
%make_build

%install
%qmake5_install

%post
/sbin/ldconfig
%{_libexecdir}/manage-groups add account-github || :
systemctl-user try-restart msyncd.service || :

%posttrans
# Pre-4.6 SailfishOS resolves theme icons from the legacy meegotouch tree.
# If that theme exists, point it at the packaged silica icons.
theme_dir=%{_datadir}/themes/sailfish-default
legacy_dir="$theme_dir/meegotouch"
if [ -d "$legacy_dir" ]; then
    for iconname in github-mark-white.png github-mark.png; do
        for icon in "$theme_dir"/silica/*/icons/${iconname}; do
            [ -e "$icon" ] || continue
            scale="$(basename "$(dirname "$(dirname "$icon")")")"
            target_dir="$legacy_dir/$scale/icons"
            [ -d "$target_dir" ] || continue
            ln -sfn "../../../silica/${scale}/icons/${iconname}" \
                "$target_dir/${iconname}"
        done
    done
fi

%postun
/sbin/ldconfig
if [ "$1" -eq 0 ]; then
    theme_dir=%{_datadir}/themes/sailfish-default
    legacy_dir="$theme_dir/meegotouch"
    if [ -d "$legacy_dir" ]; then
        for iconname in github-mark-white.png github-mark.png; do
            for icon in "$legacy_dir"/*/icons/${iconname}; do
                [ -L "$icon" ] || continue
                rm -f "$icon"
            done
        done
    fi
    %{_libexecdir}/manage-groups remove account-github || :
fi

%files
%license LICENSES/BSD-3-Clause.txt
%license LICENSES/LGPL-2.1-or-later.txt
#%%{_libdir}/libgithubcommon.so.*
#%%exclude %%{_libdir}/libgithubcommon.so
%{_libdir}/libgithubbuteocommon.so.*
%exclude %{_libdir}/libgithubbuteocommon.so
%{_datadir}/accounts/providers/github.provider
%{_datadir}/accounts/services/github-notifications.service
%{_datadir}/accounts/ui/GitHubSettingsDisplay.qml
%{_datadir}/accounts/ui/github.qml
%{_datadir}/accounts/ui/github-settings.qml
%{_datadir}/accounts/ui/github-update.qml
%{_libdir}/qt5/qml/com/jolla/settings/accounts/github/*
%{_datadir}/translations/settings-accounts-github_eng_en.qm
%{_datadir}/themes/sailfish-default/silica/*/icons/github-mark*.png
%{_libdir}/buteo-plugins-qt5/oopp/libgithub-notifications-client.so
%config %{_sysconfdir}/buteo/profiles/client/github-notifications.xml
%config %{_sysconfdir}/buteo/profiles/sync/github.Notifications.xml
#%%{_libdir}/qt5/qml/com/jolla/eventsview/github/*
%{_datadir}/lipstick/eventfeed/github-delegate.qml
%{_datadir}/lipstick/eventfeed/GitHubFeedItem.qml
%{_datadir}/translations/lipstick-jolla-home-github_eng_en.qm
%{_datadir}/translations/lipstick-jolla-home-github-notifications_eng_en.qm

%files -n sailfish-account-github-ts-devel
%{_datadir}/translations/source/settings-accounts-github.ts
%{_datadir}/translations/source/lipstick-jolla-home-github.ts
%{_datadir}/translations/source/lipstick-jolla-home-github-notifications.ts
