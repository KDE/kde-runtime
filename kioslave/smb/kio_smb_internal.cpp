/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_internal.cpp
//
// Abstract:    Utility class implementation used by SMBSlave
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program; see the file COPYING.  If not, please obtain
//     a copy from http://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "kio_smb.h"
#include "kio_smb_internal.h"

#include <qtextcodec.h>

#include <kconfig.h>
#include <kglobal.h>


//===========================================================================
// SMBUrl Function Implementation
//===========================================================================

//-----------------------------------------------------------------------
SMBUrl::SMBUrl()
{
    m_type = SMBURLTYPE_UNKNOWN;
}

//-----------------------------------------------------------------------
SMBUrl::SMBUrl(const KURL& kurl)
    : KURL(kurl)
  //-----------------------------------------------------------------------
{
    updateCache();
}


//-----------------------------------------------------------------------
void SMBUrl::addPath(const QString &filedir)
{
    KURL::addPath(filedir);
    updateCache();
}

//-----------------------------------------------------------------------
bool SMBUrl::cd(const QString &filedir)
{
    if (!KURL::cd(filedir))
        return false;
    updateCache();
    return true;
}

//-----------------------------------------------------------------------
void SMBUrl::updateCache()
  //-----------------------------------------------------------------------
{
    cleanPath();

    // SMB URLs are UTF-8 encoded
    kdDebug(KIO_SMB) << "updateCache " << KURL::path() << endl;
    if (KURL::url() == "smb:/")
        m_surl = "smb://";
    else {
        QString surl = "smb://";
        if (KURL::hasUser()) {
            surl += KURL::encode_string(KURL::user(), 106);
            if (KURL::hasPass()) {
                surl += ":" + KURL::encode_string(KURL::pass(), 106);
            }
            surl += "@";
        }
        surl += KURL::encode_string(KURL::host().upper(), 106);
        surl += KURL::encode_string(KURL::path(), 106);
        m_surl = surl.utf8();
    }
    m_type = SMBURLTYPE_UNKNOWN;
    // update m_type
    (void)getType();
}

//-----------------------------------------------------------------------
SMBUrlType SMBUrl::getType() const
  // Returns the type of this SMBUrl:
  //   SMBURLTYPE_UNKNOWN  - Type could not be determined. Bad SMB Url.
  //   SMBURLTYPE_ENTIRE_NETWORK - "smb:/" is entire network
  //   SMBURLTYPE_WORKGROUP_OR_SERVER - "smb:/mygroup" or "smb:/myserver"
  //   SMBURLTYPE_SHARE_OR_PATH - "smb:/mygroupe/mymachine/myshare/mydir"
  //-----------------------------------------------------------------------
{
    if(m_type != SMBURLTYPE_UNKNOWN)
        return m_type;

    if (protocol() != "smb")
    {
        m_type = SMBURLTYPE_UNKNOWN;
        return m_type;
    }

    if (path(1) == "/")
    {
        if (host().isEmpty())
            m_type = SMBURLTYPE_ENTIRE_NETWORK;
        else
            m_type = SMBURLTYPE_WORKGROUP_OR_SERVER;
        return m_type;
    }

    // Check for the path if we get this far
    m_type = SMBURLTYPE_SHARE_OR_PATH;

    return m_type;
}

