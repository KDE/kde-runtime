/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 *
 * This file is part of the Nepomuk KDE project.
 * Copyright (C) 2006 Sebastian Trueg <trueg@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _KNEPREGCORE_EXPORT_H_
#define _KNEPREGCORE_EXPORT_H_

#ifdef KNEPREGCORE_IS_SHARED_AGAIN

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

/* We use _WIN32/_WIN64 instead of Q_OS_WIN so that this header can be used from C files too */
#if defined _WIN32 || defined _WIN64

#ifndef KNEPREGCORE_EXPORT
# if defined(MAKE_KNEPREGCORE_LIB)
   /* We are building this library */ 
#  define KNEPREGCORE_EXPORT KDE_EXPORT
# else
   /* We are using this library */ 
#  define KNEPREGCORE_EXPORT KDE_IMPORT
# endif
#endif

#else /* UNIX */

#define KNEPREGCORE_EXPORT KDE_EXPORT

#endif
#else
# define KNEPREGCORE_EXPORT
#endif

#endif
