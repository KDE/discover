# FindGObject.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GObject.
#
#   SPDX-FileCopyrightText: 2016 Evan Nemerson <evan@nemerson.com>
#
#   SPDX-License-Identifier: MIT

find_package(PkgConfig)

set(GObject_DEPS
  GLib)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GObject_PKG gobject-2.0)
endif()

find_library(GObject_LIBRARY gobject-2.0 HINTS ${GObject_PKG_LIBRARY_DIRS})
set(GObject gobject-2.0)

if(GObject_LIBRARY AND NOT GObject_FOUND)
  add_library(${GObject} SHARED IMPORTED)
  set_property(TARGET ${GObject} PROPERTY IMPORTED_LOCATION "${GObject_LIBRARY}")
  set_property(TARGET ${GObject} PROPERTY INTERFACE_COMPILE_OPTIONS "${GObject_PKG_CFLAGS_OTHER}")

  find_path(GObject_INCLUDE_DIR "gobject/gobject.h"
    HINTS ${GObject_PKG_INCLUDE_DIRS})

  find_package(GLib)
  set(GObject_VERSION "${GLib_VERSION}")

  list(APPEND GObject_DEPS_FOUND_VARS "GLib_FOUND")
  list(APPEND GObject_INCLUDE_DIRS ${GLib_INCLUDE_DIRS})
  set_property(TARGET ${GObject} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GObject_INCLUDE_DIR}")

  set_property (TARGET "${GObject}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${GLib}")
endif()

find_program(GLib_GENMARSHAL glib-genmarshal)
if(GLib_GENMARSHAL AND NOT GLib_FOUND)
  add_executable(glib-genmarshal IMPORTED)
  set_property(TARGET glib-genmarshal PROPERTY IMPORTED_LOCATION "${GLib_GENMARSHAL}")
endif()

find_program(GLib_MKENUMS glib-mkenums)
if(GLib_MKENUMS AND NOT GLib_FOUND)
  add_executable(glib-mkenums IMPORTED)
  set_property(TARGET glib-mkenums PROPERTY IMPORTED_LOCATION "${GLib_MKENUMS}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GObject
    REQUIRED_VARS
      GObject_LIBRARY
      GObject_INCLUDE_DIRS
      ${GObject_DEPS_FOUND_VARS}
    VERSION_VAR
      GObject_VERSION)

unset(GObject_DEPS_FOUND_VARS)
