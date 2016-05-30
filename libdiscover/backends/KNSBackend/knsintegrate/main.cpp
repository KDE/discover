/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDebug>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("knsfile"), QStringLiteral("*.knsrc file"), QStringLiteral("knsfile"));
    parser.addPositionalArgument(QStringLiteral("iconName"), QStringLiteral("Icon to use"), QStringLiteral("icon"));
    parser.addPositionalArgument(QStringLiteral("category"), QStringLiteral("Category display name (categoryA/categoryB)"), QStringLiteral("category"));
    parser.addHelpOption();
    parser.process(app);

    const auto args = parser.positionalArguments();
    if (args.count()!=3) {
        parser.showHelp(1);
    }

    const QString knsFile = args.at(0);
    const QString iconName = args.at(1);
    const QStringList categoryName = args.at(2).split(QLatin1Char('/'));

    const QString outputName = QStringLiteral("kns%1-backend").arg(QFileInfo(knsFile).baseName());

    {
        QFile f(outputName + QStringLiteral("-categories.xml"));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(stderr) << "Could not open file to write:" << f.fileName() << '\n';
            return 1;
        }

        const QString filter = QStringLiteral(
            "      <Include>\n"
            "        <Or>\n"
            "          <Category>") + knsFile + QStringLiteral("</Category>\n"
            "        </Or>\n"
            "      </Include>\n");

        QTextStream fs(&f);
        fs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<Menu>\n"
            "  <Menu>\n"
            "    <Name>" << categoryName[0] << "</Name>\n"
            "    <Addons/>\n"
            "    <Icon>plasma</Icon>\n"
            "    <Menu>\n"
            "      <Name>" << categoryName[1] << "</Name>\n"
            "      <Icon>" << iconName << "</Icon>\n"
            "      <ShowTechnical>true</ShowTechnical>\n"
            << filter <<
            "    </Menu>\n"
            << filter <<
            "  </Menu>\n"
            "</Menu>\n";
    }

    {
        QFile df(outputName + QStringLiteral(".desktop"));
        if (!df.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(stderr) << "Could not open file to write:" << df.fileName() << '\n';
            return 1;
        }

        QTextStream dfs(&df);
        dfs <<
            "[Desktop Entry]\n"
            "Type=Service\n"
            "Icon=" << iconName << "\n"
            "X-KDE-Library=kns-backend\n"
            "X-KDE-PluginInfo-Name=" << outputName <<"\n"
            "X-Muon-Arguments=" << knsFile << "\n";
    }
    return 0;
}
