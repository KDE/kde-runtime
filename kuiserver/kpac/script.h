/* 
   Copyright (c) 2003 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#ifndef KPAC_SCRIPT_H
#define KPAC_SCRIPT_H

#include <qstring.h>

#include <kjs/interpreter.h>

class KURL;

namespace KPAC
{
    class Script
    {
    public:
        class Error
        {
        public:
            Error( const QString& message )
                : m_message( message ) {}
            const QString& message() const { return m_message; }

        private:
            QString m_message;
        };

        Script( const QString& code );
        QString evaluate( const KURL& );

    private:
        KJS::Interpreter m_interpreter;
    };
}

#endif // KPAC_SCRIPT_H

// vim: ts=4 sw=4 et
