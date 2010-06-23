/***************************************************************************
 *   Copyright (C) 2008-2010 by Daniel Nicoletti                           *
 *   dantti85-pk@yahoo.com.br                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include "MuonStrings.h"

#include <KLocale>

#include <KDebug>

QHash<QString, QString> groupMap()
{
    QHash<QString, QString> hash;
    hash["admin"] = i18nc("Human-readable name for the Debian package section \"admin\"",
                          "System Administration");
    hash["base"] = i18nc("Human-readable name for the Debian package section \"base\"",
                         "Base System");
    hash["cli-mono"] = i18nc("Human-readable name for the Debian package section \"cli-mono\"",
                             "Mono/CLI Infrastructure");
    hash["comm"] = i18nc("Human-readable name for the Debian package section \"comm\"",
                         "Communication");
    hash["database"] = i18nc("Human-readable name for the Debian package section \"database\"",
                             "Databases");
    hash["devel"] = i18nc("Human-readable name for the Debian package section \"devel\"",
                          "Development");
    hash["doc"] = i18nc("Human-readable name for the Debian package section \"doc\"",
                         "Documentation");
    hash["debug"] = i18nc("Human-readable name for the Debian package section \"debug\"",
                          "Debug");
    hash["editors"] = i18nc("Human-readable name for the Debian package section \"editors\"",
                            "Editors");
    hash["electronics"] = i18nc("Human-readable name for the Debian package section \"electronics\"",
                                "Electronics");
    hash["embedded"] = i18nc("Human-readable name for the Debian package section \"embedded\"",
                             "Embedded Devices");
    hash["fonts"] = i18nc("Human-readable name for the Debian package section \"fonts\"",
                          "Fonts");
    hash["games"] = i18nc("Human-readable name for the Debian package section \"games\"",
                          "Games and Amusement");
    hash["gnome"] = i18nc("Human-readable name for the Debian package section \"gnome\"",
                          "GNOME Desktop Environment");
    hash["graphics"] = i18nc("Human-readable name for the Debian package section \"graphics\"",
                             "Graphics");
    hash["gnu-r"] = i18nc("Human-readable name for the Debian package section \"gnu-r\"",
                                "GNU R Statistical System");
    hash["gnustep"] = i18nc("Human-readable name for the Debian package section \"gnustep\"",
                            "Gnustep Desktop Environment");
    hash["hamradio"] = i18nc("Human-readable name for the Debian package section \"hamradio\"",
                                "Amateur Radio");
    hash["haskell"] = i18nc("Human-readable name for the Debian package section \"haskell\"",
                                "Haskell Programming Language");
    hash["httpd"] = i18nc("Human-readable name for the Debian package section \"httpd\"",
                                "Web Servers");
    hash["interpreters"] = i18nc("Human-readable name for the Debian package section \"interpreters\"",
                                 "Interpreted Computer Languages");
    hash["java"] = i18nc("Human-readable name for the Debian package section \"java\"",
                         "Java Programming Language");
    hash["kde"] = i18nc("Human-readable name for the Debian package section \"kde\"",
                        "KDE Desktop Environment");
    hash["kernel"] = i18nc("Human-readable name for the Debian package section \"kernel\"",
                           "Kernel and Modules");
    hash["libdevel"] = i18nc("Human-readable name for the Debian package section \"libdevel\"",
                             "Libraries - Development");
    hash["libs"] = i18nc("Human-readable name for the Debian package section \"libs\"",
                         "Libraries");
    hash["lisp"] = i18nc("Human-readable name for the Debian package section \"lisp\"",
                                "Lisp Programming Language");
    hash["localization"] = i18nc("Human-readable name for the Debian package section \"localization\"",
                                 "Localization");
    hash["mail"] = i18nc("Human-readable name for the Debian package section \"mail\"",
                         "Email");
    hash["math"] = i18nc("Human-readable name for the Debian package section \"math\"",
                         "Mathematics");
    hash["misc"] = i18nc("Human-readable name for the Debian package section \"misc\"",
                         "Miscellaneous - Text-based");
    hash["net"] = i18nc("Human-readable name for the Debian package section \"net\"",
                         "Networking");
    hash["news"] = i18nc("Human-readable name for the Debian package section \"news\"",
                         "Newsgroups");
    hash["ocaml"] = i18nc("Human-readable name for the Debian package section \"ocaml\"",
                          "OCaml Programming Language");
    hash["oldlibs"] = i18nc("Human-readable name for the Debian package section \"oldlibs\"",
                            "Libraries - Old");
    hash["otherosfs"] = i18nc("Human-readable name for the Debian package section \"otherosfs\"",
                              "Cross Platform");
    hash["perl"] = i18nc("Human-readable name for the Debian package section \"perl\"",
                         "Perl Programming Language");
    hash["php"] = i18nc("Human-readable name for the Debian package section \"php\"",
                        "PHP Programming Language");
    hash["python"] = i18nc("Human-readable name for the Debian package section \"python\"",
                           "Python Programming Language");
    hash["ruby"] = i18nc("Human-readable name for the Debian package section \"ruby\"",
                         "Ruby Programming Language");
    hash["science"] = i18nc("Human-readable name for the Debian package section \"science\"",
                            "Science");
    hash["shells"] = i18nc("Human-readable name for the Debian package section \"shells\"",
                           "Shells");
    hash["sound"] = i18nc("Human-readable name for the Debian package section \"sound\"",
                          "Multimedia");
    hash["mail"] = i18nc("Human-readable name for the Debian package section \"mail\"",
                         "Email");
    hash["tex"] = i18nc("Human-readable name for the Debian package section \"tex\"",
                        "TeX Authoring");
    hash["text"] = i18nc("Human-readable name for the Debian package section \"text\"",
                         "Word Processing");
    hash["utils"] = i18nc("Human-readable name for the Debian package section \"utils\"",
                         "Utilities");
    hash["vcs"] = i18nc("Human-readable name for the Debian package section \"vcs\"",
                        "Version Control Systems");
    hash["video"] = i18nc("Human-readable name for the Debian package section \"video\"",
                         "Video Software");
    hash["web"] = i18nc("Human-readable name for the Debian package section \"web\"",
                        "Internet");
    hash["x11"] = i18nc("Human-readable name for the Debian package section \"x11\"",
                        "Miscellaneous - Graphical");
    hash["xfce"] = i18nc("Human-readable name for the Debian package section \"xfce\"",
                         "Xfce Desktop Environment");
    hash["zope"] = i18nc("Human-readable name for the Debian package section \"zope\"",
                         "Zope/Plone Environment");
    hash["unknown"] = i18nc("Human-readable name for the Debian package section \"unknown\"",
                            "Unknown");
    hash["alien"] = i18nc("Human-readable name for the Debian package section \"alien\"",
                          "Converted from RPM by Alien");
    hash["translations"] = i18nc("Human-readable name for the Debian package section \"translations\"",
                                 "Internationalization and Localization");
    hash["metapackages"] = i18nc("Human-readable name for the Debian package section \"metapackages\"",
                                 "Meta Packages");
    hash["non-us"] = i18nc("Debian package section \"non-US\", for packages that cannot be shipped in the US",
                           "Restricted On Export");
    hash["non-free"] = i18nc("Human-readable name for the Debian package section \"non-free\"",
                         "Non-free");
    hash["contrib"] = i18nc("Human-readable name for the Debian package section \"contrib\"",
                            "Contrib");
    return hash;
}

QString MuonStrings::groupName(const QString &name)
{
    QHash<QString, QString> groups = groupMap();
    QString group;
    QString prefix;
    QString suffix;

    if (name.contains('/')) {
        QStringList split = name.split('/');
        prefix = split[0];
        suffix = split[1];

        if (groups.contains(suffix)) {
            return groups[suffix];
        }
    } else {
        if (groups.contains(name)) {
            return groups[name];
        }
    }
    return QString();
}

QString MuonStrings::groupKey(const QString &text)
{
    QHash<QString, QString> groups = groupMap();

    return groups.key(text);
}

QHash<int, QString> stateMap()
{
    QHash<int, QString> hash;
    hash[QApt::Package::ToKeep] = i18n("Not Installed");
    hash[QApt::Package::Installed] = i18n("Installed");
    hash[QApt::Package::Upgradeable] = i18n("Upgradeable");
    hash[QApt::Package::NowBroken] = i18n("Broken");

    return hash;
}

QString MuonStrings::packageStateName(QApt::Package::PackageState state)
{
    QHash<int, QString> states = stateMap();

    if (states.contains(state)) {
        return states[state];
    }
    return QString();
}

QApt::Package::PackageState MuonStrings::packageStateKey(const QString &text)
{
    QHash<int, QString> states = stateMap();

    return (QApt::Package::PackageState)states.key(text);
}
