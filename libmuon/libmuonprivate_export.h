/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef MUONPRIVATE_EXPORT_H
#define MUONPRIVATE_EXPORT_H

// needed for KDE_EXPORT and KDE_IMPORT macros
#include <kdemacros.h>

#ifndef MUONPRIVATE_EXPORT
# if defined(MAKE_MUONPRIVATE_LIB)
   // We are building this library
#  define MUONPRIVATE_EXPORT KDE_EXPORT
# else
   // We are using this library
#  define MUONPRIVATE_EXPORT KDE_IMPORT
# endif
#endif

# ifndef MUONPRIVATE_EXPORT_DEPRECATED
#  define MUONPRIVATE_EXPORT_DEPRECATED KDE_DEPRECATED MUONPRIVATE_EXPORT
# endif

#endif
