# This module defines the following variables:
#
#  systemd-sysupdate_FOUND - true if found
#  systemd-sysupdate_PATH - path to the bin (only when found)
#
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

include(ProgramFinder)
program_finder(systemd-sysupdate PATHS /usr/lib/systemd)
