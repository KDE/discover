/*
 *   SPDX-FileCopyrightText: 2013-2026 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

template<typename T>
class GLibHolder
{
public:
    GLibHolder(T *object = nullptr)
        : m_object(object)
    {
        if (object) {
            g_object_ref(object);
        }
    }

    ~GLibHolder()
    {
        if (m_object) {
            g_object_unref(m_object);
        }
    }

    GLibHolder(const GLibHolder &other)
        : GLibHolder(other.m_object)
    {
    }

    GLibHolder(GLibHolder &&other)
        : m_object(other.m_object)
    {
        other.m_object = nullptr;
    }

    T *get() const
    {
        return m_object;
    }

    T **ref()
    {
        return &m_object;
    }

    void adopt(T *ptr)
    {
        if (ptr == m_object)
            return;
        if (m_object) {
            g_object_unref(m_object);
        }
        m_object = ptr;
    }

    operator bool() const
    {
        return m_object != nullptr;
    }

private:
    T *m_object = nullptr;
};
