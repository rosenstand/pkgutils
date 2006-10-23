/*
 This file was a part of GNU coreutils, a bit modified for pkgutils.
 Copyright (C) Anton Vorontsov <cbou@mail.ru>

 filemode.c -- make a string describing file modes
 Copyright (C) 1985, 1990, 1993, 1998-2000, 2004 Free Software Foundation, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#pragma once

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>

void mode_string (mode_t mode, char *str);
