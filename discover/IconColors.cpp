/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "IconColors.h"
#include <QIcon>
#include <vector>
#include <QDebug>

// #define OUTPUT_PIXMAP_DEBUG

IconColors::IconColors(QObject* parent)
    : QObject(parent)
{}

QString IconColors::iconName() const
{
    return m_iconName;
}

void IconColors::setIconName(const QString& name)
{
    if (m_iconName != name) {
        m_iconName = name;
    }
}

QColor IconColors::dominantColor() const
{
    const QImage img = QIcon::fromTheme(m_iconName).pixmap({32, 32}).toImage();
    const int tolerance = 10;
    QVector<uint> hue(360/tolerance, 0);

#ifdef OUTPUT_PIXMAP_DEBUG
    QImage thing(img.size()+QSize(0,1), QImage::Format_ARGB32);
    thing.fill(Qt::white);
#endif

    for (int w=0, cw=img.width(); w<cw; ++w) {
        for (int h=0, ch=img.height(); h<ch; ++h) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
            const QColor c = img.pixelColor(w, h);
#else
            const QColor c(img.pixel(w, h));
#endif

            if (c.value()>150 && c.saturation()>20 && c.hue()>=0 && c.alpha()>200) {
                hue[c.hue()/tolerance]++;

#ifdef OUTPUT_PIXMAP_DEBUG
//                 qDebug() << "adopting" << w << "x" << h << c.name() << c.hue();
//                 thing.setPixelColor(w, h, c);
                thing.setPixelColor(w, h, QColor::fromHsv(tolerance*(c.hue()/tolerance), 220, 220));
#endif
            }
        }
    }

    uint dominantHue = 0, biggestAmount = 0;
    for(int i=0; i<hue.size(); ++i) {
        if (hue[i]>biggestAmount) {
            biggestAmount = hue[i];
            dominantHue = i;
        }
    }

    QColor ret = QColor::fromHsv((dominantHue*tolerance + tolerance/2) % 360, 255, 255);

#ifdef OUTPUT_PIXMAP_DEBUG
    qDebug() << "dominant" << dominantHue << hue[dominantHue] << "~=" << ((100*hue[dominantHue])/(img.width()*img.height())) << "% " << m_iconName;
    thing.setPixelColor(0, img.height(), ret);
    thing.save("/tmp/"+m_iconName+".png");
#endif

    return ret;
}
