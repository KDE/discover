/***************************************************************************
 *   Copyright ?? 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef UTILS_H
#define UTILS_H

#include <functional>
#include <QString>
#include <QElapsedTimer>
#include <QScopeGuard>

class OneTimeAction : public QObject
{
public:
    OneTimeAction(const std::function<void()> &func, QObject* parent) : QObject(parent), m_function(func) {}

    void trigger() {
        m_function();
        deleteLater();
    }

private:
    std::function<void()> m_function;
};

template <typename T, typename Q, typename _UnaryOperation>
static T kTransform(const Q &input, _UnaryOperation op)
{
    T ret;
    ret.reserve(input.size());
    for(const auto& v : input) {
        ret += op(v);
    }
    return ret;
}

template <typename T, typename Q, typename _UnaryOperation>
static T kAppend(const Q &input, _UnaryOperation op)
{
    T ret;
    ret.reserve(input.size());
    for(const auto& v : input) {
        ret.append(op(v));
    }
    return ret;
}

template <typename T, typename Q, typename _UnaryOperation>
static T kFilter(const Q &input, _UnaryOperation op)
{
    T ret;
    for(const auto& v : input) {
        if (op(v))
            ret += v;
    }
    return ret;
}

template <typename Q, typename W>
static int kIndexOf(const Q& list, W func)
{
    int i = 0;
    for (auto it = list.constBegin(), itEnd = list.constEnd(); it!=itEnd; ++it) {
        if (func(*it))
            return i;
        ++i;
    }
    return -1;
}

template <typename Q, typename W>
static bool kContains(const Q& list, W func)
{ return std::any_of(list.begin(), list.end(), func); }

template <typename T>
static QVector<T> kSetToVector(const QSet<T> & set)
{
    QVector<T> ret;
    ret.reserve(set.size());
    for(auto &x: set)
        ret.append(x);
    return ret;
}

template <typename T>
static QSet<T> kToSet(const QVector<T> & set)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QSet<T>(set.begin(), set.end());
#else
    QSet<T> ret;
    ret.reserve(set.size());
    for(auto &x: set)
        ret.insert(x);
    return ret;
#endif
}

template <typename T>
static QSet<T> kToSet(const QList<T> & set)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QSet<T>(set.begin(), set.end());
#else
    return set.toSet();
#endif
}

class ElapsedDebug : private QElapsedTimer
{
public:
    ElapsedDebug(const QString &name = QStringLiteral("<unnamed>")) : m_name(name) { start(); }
    ~ElapsedDebug() { qDebug("elapsed %s: %lld!", m_name.toUtf8().constData(), elapsed()); }
    void step(const QString &step) { qDebug("step %s(%s): %lld!", m_name.toUtf8().constData(), qPrintable(step), elapsed()); }

    QString m_name;
};

#endif
