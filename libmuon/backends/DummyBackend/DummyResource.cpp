/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "DummyResource.h"
#include <krandom.h>
#include <QDesktopServices>
#include <QStringList>

DummyResource::DummyResource(const QString& name, bool isTechnical, AbstractResourcesBackend* parent)
    : AbstractResource(parent)
    , m_name(name)
    , m_state(State::Broken)
    , m_addons(QList<PackageState>() << PackageState("a", "aaaaaa", false) << PackageState("b", "aaaaaa", false) << PackageState("c", "aaaaaa", false))
    , m_isTechnical(isTechnical)
{
    if(KRandom::random() % 2)
        m_screenshot = QUrl("http://www.kde.org/stuff/clipart/klogo-official-oxygen-128x128.png");
}

QList<PackageState> DummyResource::addonsInformation()
{
    return m_addons;
}

QString DummyResource::availableVersion() const
{
    return "3.0";
}

QStringList DummyResource::categories()
{
    return QStringList("dummy");
}

QString DummyResource::comment()
{
    return "comment "+name()+"...";
}

int DummyResource::downloadSize()
{
    return 123;
}

QUrl DummyResource::homepage()
{
    return QUrl("http://kde.org");
}

QString DummyResource::icon() const
{
    return isTechnical() ? "kalarm" : "kmail";
}

QString DummyResource::installedVersion() const
{
    return "2.3";
}

QString DummyResource::license()
{
    return "GPL";
}

QString DummyResource::longDescription()
{
    return "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur ultricies consequat nulla, ut vulputate nulla ultricies ac. Suspendisse lacinia commodo lacus, non tristique mauris dictum vitae. Sed adipiscing augue nec nisi aliquet viverra. Etiam sit amet nulla in tellus consectetur feugiat. Cras in sem tortor. Fusce a nulla at justo accumsan gravida. Maecenas dui felis, lacinia at ornare sed, aliquam et purus. Sed ut sagittis lacus. Etiam dictum pharetra rhoncus. Suspendisse auctor orci ipsum. Pellentesque vitae urna nec felis consequat lobortis dictum in urna. Phasellus a mi ac leo adipiscing varius eget a felis. Cras magna augue, commodo sed placerat vel, tempus vel ligula. In feugiat quam quis est lobortis sed accumsan nunc malesuada. Mauris quis massa sit amet felis tempus suscipit a quis diam.\n\n"

    "Aenean quis nulla erat, vel sagittis sem. Praesent vitae mauris arcu. Cras porttitor, ante at scelerisque sodales, nibh felis consectetur orci, ut hendrerit urna urna non urna. Duis eu magna id mi scelerisque adipiscing. Aliquam sed quam in eros sodales accumsan. Phasellus tempus sagittis suscipit. Aliquam rutrum dictum justo ut viverra. Nulla felis sem, molestie sed scelerisque non, consequat vitae nulla. Aliquam ullamcorper malesuada mi, vel vestibulum magna vulputate eget. In hac habitasse platea dictumst. Cras sed lacus dui, vel semper sem. Aenean sodales porta leo vel fringilla.\n\n"

    "Ut tempus massa et urna porta non mollis metus ultricies. Duis nec nulla ac metus auctor porta id et mi. Mauris aliquam nibh a ligula malesuada sed tincidunt nibh varius. Sed felis metus, porta et adipiscing non, faucibus id leo. Donec ipsum nibh, hendrerit eget aliquam nec, tempor ut mauris. Suspendisse potenti. Vestibulum scelerisque adipiscing libero tristique eleifend. Donec quis tortor eget elit mollis iaculis ac sit amet nisi. Proin non massa sed nunc rutrum pellentesque. Sed dui lectus, laoreet sed condimentum id, commodo sed urna.\n\n"

    "Praesent tincidunt mattis massa mattis porta. Nullam posuere neque at mauris vestibulum vitae elementum leo sodales. Quisque condimentum lectus in libero luctus egestas. Fusce tempor neque ac dui tincidunt eget viverra quam suscipit. In hac habitasse platea dictumst. Etiam metus mi, adipiscing nec suscipit id, aliquet sed sem. Duis urna ligula, ornare sed vestibulum vel, molestie ac nisi. Morbi varius iaculis ligula. Nunc in augue leo, sit amet aliquam elit. Suspendisse rutrum sem diam. Proin eu orci nisl. Praesent porttitor dignissim est, id fermentum arcu venenatis vitae.\n\n"

    "Integer in sapien eget quam vulputate lobortis. Morbi nibh elit, elementum vitae vehicula sed, consequat nec erat. Donec placerat porttitor est ut dapibus. Fusce augue orci, dictum et convallis vel, blandit eu tortor. Phasellus non eros nulla. In iaculis nulla fermentum nulla gravida eu mattis purus consectetur. Integer dui nunc, sollicitudin ac tincidunt nec, hendrerit bibendum nunc. Proin sit amet augue ac velit egestas varius. Sed eu ante quis orci vestibulum sagittis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Phasellus vitae urna odio, at molestie leo. In convallis neque vel mi dictum convallis lobortis turpis sagittis.\n\n";
}

QString DummyResource::name()
{
    return m_name;
}

QString DummyResource::origin() const
{
    return "DummySource1";
}

QString DummyResource::packageName() const
{
    return m_name;
}

QUrl DummyResource::screenshotUrl()
{
    return m_screenshot;
}

QUrl DummyResource::thumbnailUrl()
{
    return m_screenshot;
}

QString DummyResource::section()
{
    return "dummy";
}

AbstractResource::State DummyResource::state()
{
    return m_state;
}

void DummyResource::fetchChangelog()
{
    emit changelogFetched(longDescription().left(200));
}

void DummyResource::setState(AbstractResource::State state)
{
    m_state = state;
    emit stateChanged();
}
void DummyResource::invokeApplication() const
{
    QDesktopServices d;
    d.openUrl(QUrl("https://projects.kde.org/projects/extragear/sysadmin/muon"));
}
