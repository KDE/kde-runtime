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

#ifndef KERAMIK_CONF_H
#define KERAMIK_CONF_H

class QCheckBox;

class KeramikStyleConfig: public QWidget
{
	Q_OBJECT
public:
	KeramikStyleConfig(QWidget* parent);
	~KeramikStyleConfig();

	//This signal and the next two slots are the plugin
	//page interface
signals:
	void changed(bool);

public slots:
	void save();
	void defaults();

	//Everything below this is internal.
protected slots:
	void updateChanged();

protected:
	//We store settings directly in widgets to
	//avoid the hassle of sync'ing things
	//QCheckBox* highlightLineEdits;
	QCheckBox* animateProgressBar;
	QCheckBox* highlightScrollBar;

	//Original settings, for accurate dirtiness tracking
	//bool       origHlLineEdit;
	bool       origAnimProgressBar;
	bool       origHlScrollbar;
};

#endif
