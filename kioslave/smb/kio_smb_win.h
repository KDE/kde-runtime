/*
Copyright 2009  Carlo Segato <brandon.ml@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef KIO_SMB_WIN_H
#define KIO_SMB_WIN_H

#include <kio/slavebase.h>
#include <kio/forwardingslavebase.h>

#include <windows.h>
#include <winnetwk.h>

#define KIO_SMB                     7106

class SMBSlave : public KIO::ForwardingSlaveBase
{
    public:
        SMBSlave(const QByteArray &pool, const QByteArray &app);
        ~SMBSlave();
        bool rewriteUrl(const QUrl &url, QUrl &newUrl);
        void listDir(const QUrl &url);
        void stat(const QUrl &url);
    private:
        void enumerateResources(LPNETRESOURCE lpnr, bool show_servers = false);

};

#endif
