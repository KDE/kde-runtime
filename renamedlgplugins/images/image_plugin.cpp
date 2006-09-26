/*
 * Copyright (C)  2001, 2006 Holger Freyther <freyther@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <kio/renamedlgplugin.h>
#include <kio/global.h>

#include <kdebug.h>
#include <kgenericfactory.h>
#include <kiconloader.h>


#include <qlabel.h>
#include <qdialog.h>
#include <qwidget.h>
#include <qstringlist.h>

//Added by qt3to4:
#include <QGridLayout>
#include <qlayout.h>

#include <sys/types.h>

#include "imagevisualizer.h"

class ImagePlugin : public KIO::RenameDlgPlugin {
public:
    ImagePlugin( QDialog *dialog, const QStringList & );
    virtual bool wantToHandle( KIO::RenameDlg_Mode, const KIO::RenameDlgPlugin::FileItem&,
                               const KIO::RenameDlgPlugin::FileItem& ) const;
    virtual void handle( KIO::RenameDlg_Mode, const KIO::RenameDlgPlugin::FileItem&,
                         const KIO::RenameDlgPlugin::FileItem& );
};

ImagePlugin::ImagePlugin( QDialog *dialog, const QStringList & )
  : RenameDlgPlugin( dialog)
{
}

bool ImagePlugin::wantToHandle( KIO::RenameDlg_Mode mode, const KIO::RenameDlgPlugin::FileItem&,
                                const KIO::RenameDlgPlugin::FileItem& ) const {
    return true;
}

void ImagePlugin::handle( KIO::RenameDlg_Mode mode, const KIO::RenameDlgPlugin::FileItem& src,
                          const KIO::RenameDlgPlugin::FileItem& dst ) {
    QGridLayout *lay = new QGridLayout(this, 2, 3, 5  );
    if( mode & KIO::M_OVERWRITE ) {
        QLabel *label = new QLabel(this );
        label->setText(i18n("You want to overwrite the left picture with the one on the right.") );
        label->adjustSize();
        lay->addMultiCellWidget(label, 1, 1, 0, 2,  Qt::AlignHCenter  );
        adjustSize();
    }

    ImageVisualizer *left= new ImageVisualizer(this, dst.url() );
    ImageVisualizer *right = new ImageVisualizer( this, src.url() );

    lay->addWidget(left, 2, 0 );
    lay->addWidget(right, 2, 2 );
    adjustSize();
}

typedef KGenericFactory<ImagePlugin, QDialog> ImagePluginFactory;
K_EXPORT_COMPONENT_FACTORY( librenimageplugin, ImagePluginFactory("imagerename_plugin") )
