/*
    Copyright (c) 2008 Dirk Mueller <mueller@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/
#ifndef ATTICACLIENT_EXPORT_H
#define ATTICACLIENT_EXPORT_H

#include <QtCore/QtGlobal>

#ifndef ATTICA_EXPORT
# if defined(MAKE_ATTICA_LIB)
   /* We are building this library */
#  define ATTICA_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#  define ATTICA_EXPORT Q_DECL_IMPORT
# endif
#endif

#endif
