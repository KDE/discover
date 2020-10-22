/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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

template <typename T, typename Q>
static T kTransform(const Q &input)
{
    T ret;
    ret.reserve(input.size());
    for(const auto& v : input) {
        ret += v;
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

template <typename Q, typename W>
static bool kContainsValue(const Q& list, W value)
{ return std::find(list.begin(), list.end(), value) != list.end(); }

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
    return QSet<T>(set.begin(), set.end());
}

template <typename T>
static QSet<T> kToSet(const QList<T> & set)
{
    return QSet<T>(set.begin(), set.end());
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
