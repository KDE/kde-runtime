/*
Copyright (c) 2003 Maksim Orlovich <maksim.orlovich@kdemail.net>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <qcheckbox.h>
#include <qlayout.h>
#include <qsettings.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>

#include "keramikconf.h"

extern "C"
{
	KDE_EXPORT QWidget* allocate_kstyle_config(QWidget* parent)
	{
		return new KeramikStyleConfig(parent);
	}
}

KeramikStyleConfig::KeramikStyleConfig(QWidget* parent): QWidget(parent)
{
	//Should have no margins here, the dialog provides them
	QVBoxLayout* layout = new QVBoxLayout(this, 0, 0);
	KGlobal::locale()->insertCatalogue("kstyle_keramik_config");

	//highlightLineEdits = new QCheckBox(i18n("Highlight active lineedits"), this);
	highlightScrollBar = new QCheckBox(i18n("Highlight scroll bar handles"), this);
	animateProgressBar = new QCheckBox(i18n("Animate progress bars"), this);

	//layout->add(highlightLineEdits);
	layout->add(highlightScrollBar);
	layout->add(animateProgressBar);
	layout->addStretch(1);

	QSettings s;
	//origHlLineEdit = s.readBoolEntry("/keramik/Settings/highlightLineEdits", false);
	//highlightLineEdits->setChecked(origHlLineEdit);

	origHlScrollbar = s.readBoolEntry("/keramik/Settings/highlightScrollBar", true);
	highlightScrollBar->setChecked(origHlScrollbar);

	origAnimProgressBar = s.readBoolEntry("/keramik/Settings/animateProgressBar", false);
	animateProgressBar->setChecked(origAnimProgressBar);
	
	//connect(highlightLineEdits, SIGNAL( toggled(bool) ), SLOT( updateChanged() ) );
	connect(highlightScrollBar, SIGNAL( toggled(bool) ), SLOT( updateChanged() ) );
	connect(animateProgressBar, SIGNAL( toggled(bool) ), SLOT( updateChanged() ) );
}

KeramikStyleConfig::~KeramikStyleConfig()
{
	KGlobal::locale()->removeCatalogue("kstyle_keramik_config");
}


void KeramikStyleConfig::save()
{
	QSettings s;
	//s.writeEntry("/keramik/Settings/highlightLineEdits", highlightLineEdits->isChecked());
	s.writeEntry("/keramik/Settings/highlightScrollBar", highlightScrollBar->isChecked());
	s.writeEntry("/keramik/Settings/animateProgressBar", animateProgressBar->isChecked());
}

void KeramikStyleConfig::defaults()
{
	//highlightLineEdits->setChecked(false);
	highlightScrollBar->setChecked(true);
	animateProgressBar->setChecked(false);
	//updateChanged would be done by setChecked already
}

void KeramikStyleConfig::updateChanged()
{
	if ( /*(highlightLineEdits->isChecked() == origHlLineEdit)  &&*/
		 (highlightScrollBar->isChecked() == origHlScrollbar) &&
		 (animateProgressBar->isChecked() == origAnimProgressBar) )
		emit changed(false);
	else
		emit changed(true);
}

#include "keramikconf.moc"
