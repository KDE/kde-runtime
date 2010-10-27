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

#include <kio/renamedialogplugin.h>
#include <kio/global.h>

#include <kdebug.h>
#include <kpluginfactory.h>
#include <klocale.h>
#include <kiconloader.h>


#include <QLabel>
#include <QDialog>
#include <QWidget>
#include <QStringList>

//Added by qt3to4:
#include <QGridLayout>
#include <QLayout>

#include <sys/types.h>

#include "imagevisualizer.h"

class ImagePlugin : public KIO::RenameDialogPlugin {
public:
    ImagePlugin( QWidget *dialog, const QVariantList & );
    virtual bool wantToHandle( KIO::RenameDialog_Mode, const KIO::RenameDialogPlugin::FileItem&,
                               const KIO::RenameDialogPlugin::FileItem& ) const;
    virtual void handle( KIO::RenameDialog_Mode, const KIO::RenameDialogPlugin::FileItem&,
                         const KIO::RenameDialogPlugin::FileItem& );
};

ImagePlugin::ImagePlugin( QWidget *dialog, const QVariantList & )
  : RenameDialogPlugin(static_cast<QDialog*>(dialog))
{
}

bool ImagePlugin::wantToHandle( KIO::RenameDialog_Mode, const KIO::RenameDialogPlugin::FileItem&,
                                const KIO::RenameDialogPlugin::FileItem& ) const {
    return true;
}

void ImagePlugin::handle( KIO::RenameDialog_Mode mode, const KIO::RenameDialogPlugin::FileItem& src,
                          const KIO::RenameDialogPlugin::FileItem& dst ) {
    QGridLayout *lay = new QGridLayout( this );
    if( mode & KIO::M_OVERWRITE ) {
        QLabel *label = new QLabel(this );
        label->setText(i18n("You want to overwrite the left picture with the one on the right.") );
        label->adjustSize();
        lay->addWidget(label, 1, 0, 1, 3, Qt::AlignHCenter );
        adjustSize();
    }

    ImageVisualizer *left= new ImageVisualizer(this, dst.url() );
    ImageVisualizer *right = new ImageVisualizer( this, src.url() );

    lay->addWidget(left, 2, 0 );
    lay->addWidget(right, 2, 2 );
    adjustSize();
}


K_PLUGIN_FACTORY(ImagePluginFactory,
                 registerPlugin<ImagePlugin>();)
K_EXPORT_PLUGIN(ImagePluginFactory("imagerename_plugin"))
