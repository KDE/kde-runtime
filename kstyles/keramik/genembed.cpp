/**
A small utility to generate embedded images for Keramik, especially structured for easy recoloring...

Copyright (c) 2002, 2005 Maksim Orlovich <maksim@kde.org>

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

#include <QtCore/QCoreApplication>
#include <QtGui/QColor>
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QImage>
#include <QtCore/QMap>
#include <QtCore/QTextStream>
#include <QtCore/QVector>

#include <blitz.h>

#include <iostream>
using namespace std;

#include <string.h>
#include <math.h>

//NOTE: Use of old-style header is intentional for portability. See revisions 1.6 and 1.7

//Force-touch-embedded-revision: 2

#include "keramikimage.h"

/**
Need to generate something like this:
TargetColorAlpha, GreyAdd, SrcAlpha;

so that one can do (R*T+GreyAdd, G*T+GreyAdd, B*T+GreyAdd, SrcAlpha) as pixel values
*/


int evalSuffix(const QString &suffix)
{
	if (suffix == "-tl")
		return 0;

	if (suffix == "-tc")
		return 1;

	if (suffix == "-tr")
		return 2;

	if (suffix == "-cl")
		return 3;

	if (suffix == "-cc")
		return 4;

	if (suffix == "-cr")
		return 5;

	if (suffix == "-bl")
		return 6;

	if (suffix == "-bc")
		return 7;

	if (suffix == "-br")
		return 8;

	if (suffix == "-separator")
		return KeramikTileSeparator;

	if (suffix == "-slider1")
		return KeramikSlider1;

	if (suffix == "-slider2")
		return KeramikSlider2;

	if (suffix == "-slider3")
		return KeramikSlider3;

	if (suffix == "-slider4")
		return KeramikSlider4;

	if (suffix == "-groove1")
		return KeramikGroove1;

	if (suffix == "-groove2")
		return KeramikGroove2;

	if (suffix == "-1")
		return 1;

	if (suffix == "-2")
		return 2;

	if (suffix == "-3")
		return 3;

	return -1;
}


int main(int argc, char** argv)
{
	if (argc < 2)
		return 0;

	QCoreApplication qapp(argc, argv);
	QVector<KeramikEmbedImage> images;

	QStringList imageList;
	if (argc == 3 && (strcmp(argv[1], "--file")==0)) {
		QFile f( argv[2] );
		if (!f.open(QIODevice::ReadOnly))
			return 0;
		QByteArray ba = f.readLine();
		while (!ba.isEmpty()) {
			imageList += ba.trimmed();
			ba = f.readLine();
		}
		f.close();
	} else {
		for (int c = 1; c<argc; c++)
			imageList += argv[c];
	}

	cout<<"#include <QHash>\n\n";
	cout<<"#include \"keramikimage.h\"\n\n";

	QMap<QString, int> assignID;
	int nextID = 0;

	for(QStringList::iterator it = imageList.begin(); it != imageList.end(); ++it)
	{
		QImage input((*it));
		input = input.convertToFormat( QImage::Format_ARGB32 );

		QFileInfo fi((*it));
		QString s = fi.baseName();

		KeramikEmbedImage image;

		int pos;

		QString id = s;

		int readJustID = 0;


		if ((pos = s.lastIndexOf("-")) != -1)
		{
				int suffix = evalSuffix(s.mid(pos));
				if (suffix !=-1 )
				{
						id = s.mid(0,pos);
						readJustID = suffix;
				}
		}

		if (!assignID.contains(id))
		{
			assignID[id] = nextID;
			nextID += 256;
		}

		s.replace("-","_");


		if (s.contains("button"))
			Blitz::contrast(input, true);

		int fullID = assignID[id] + readJustID;//Subwidget..

		bool highlights = true;
		bool shadows  = true;

		float gamma    = 1.0;
		int brightAdj = 0;



		if (s.contains("toolbar") || s.contains("tab-top-active") || s.contains("menubar") )
		{
//			highlights = false;
			gamma    = 1/1.25f;
			//brightAdj = 10;
			shadows = false;
		}

		if (s.contains("scrollbar") && s.contains("groove"))
		{
			//highlights = false;
			//gamma = 1.5;
			shadows = false;
		}
			//brightAdj = -10;

		if (s.contains("scrollbar") && s.contains("slider"))
		{
			//highlights = false;
			gamma =1/0.7f;
			//shadows = false;
		}


		if (s.contains("menuitem"))
		{
			//highlights = false;
			gamma =1/0.6f;
			//shadows = false;
		}

		image.width   = input.width();
		image.height = input.height();
		image.id         = fullID;
		image.data     = reinterpret_cast<unsigned char*>(strdup(s.toLatin1()));


		bool reallySolid = true;

		int pixCount = 0;
		int pixSolid = 0;

		cout<<"static const unsigned char "<<qPrintable(s)<<"[]={\n";

		quint32* read  = reinterpret_cast< quint32* >(input.bits() );
		int size = input.width()*input.height();

		for (int pos=0; pos<size; pos++)
		{
			QRgb basePix = (QRgb)*read;

			if (qAlpha(basePix) != 255)
				reallySolid = false;
			else
				pixSolid++;

			pixCount++;
			read++;
		}

		image.haveAlpha = !reallySolid;

		images.push_back(image);

		read  = reinterpret_cast< quint32* >(input.bits() );
		for (int pos=0; pos<size; pos++)
		{
			QRgb basePix = (QRgb)*read;
			//cout<<(r*destAlpha.alphas[pos])<<"\n";
			//cout<<(int)destAlpha.alphas[pos]<<"\n";
			QColor clr(basePix);
			int h,s,v;
			clr.getHsv(&h,&s,&v);

			v=qGray(basePix);

			int targetColorAlpha = 0 , greyAdd = 0;
			//int srcAlpha = qAlpha(basePix);

			if (s>0 || v > 128)
			{ //Non-shadow
				float fv = v/255.0;
				fv = pow(fv, gamma);
				v = int(255.5*fv);


				if (s<17 && highlights) //A bit of a highligt..
				{
					float effectPortion = (16 - s)/15.0;

					greyAdd             = (int)(v/4.0 * effectPortion*1.2);
					targetColorAlpha = v - greyAdd;
				}
				else
				{
					targetColorAlpha = v;//(int)(fv*255);
					greyAdd              = 0;
				}
			}
			else
			{
				if (shadows)
				{
					targetColorAlpha = 0;
					greyAdd              = v;
				}
				else
				{
					targetColorAlpha = v;//(int)(fv*255);
					greyAdd              = 0;
				}
			}

			greyAdd+=brightAdj;

			if (reallySolid)
				cout<<targetColorAlpha<<","<<greyAdd<<",";
			else
				cout<<targetColorAlpha<<","<<greyAdd<<","<<qAlpha(basePix)<<",";
			//cout<<qRed(basePix)<<","<<qGreen(basePix)<<","<<qBlue(basePix)<<","<<qAlpha(basePix)<<",";

			if (pos%8 == 7)
				cout<<"\n";

			read++;
		}

		//cerr<<qPrintable(s)<<":"<<pixSolid<<"/"<<pixCount<<"("<<reallySolid<<")\n";

		cout<<!reallySolid<<"\n";

		cout<<"};\n\n";
	}

	cout<<"static const KeramikEmbedImage  image_db[] = {\n";

	for (int c=0; c<images.size(); c++)
	{
		cout<<"\t{ "<<(images[c].haveAlpha?"true":"false")<<","<<images[c].width<<", "<<images[c].height<<", "<<images[c].id<<", "<<(char *)images[c].data<<"},";
		cout<<"\n";
	}
	cout<<"\t{0, 0, 0, 0, 0}\n";
	cout<<"};\n\n";

	cout<<"class KeramikImageDb\n";
	cout<<"{\n";
	cout<<"public:\n";
	cout<<"\tstatic KeramikImageDb* getInstance()\n";
	cout<<"\t{\n";
	cout<<"\t\tif (!instance) instance = new KeramikImageDb;\n";
	cout<<"\t\treturn instance;\n";
	cout<<"\t}\n\n";
	cout<<"\tstatic void release()\n";
	cout<<"\t{\n";
	cout<<"\t\tdelete instance;\n";
	cout<<"\t\tinstance=0;\n";
	cout<<"\t}\n\n";
	cout<<"\tconst KeramikEmbedImage* getImage(int id)\n";
	cout<<"\t{\n";
	cout<<"\t\treturn images[id];\n";
	cout<<"\t}\n\n";
	cout<<"private:\n";
	cout<<"\tKeramikImageDb()\n";
	cout<<"\t{\n";
	cout<<"\t\timages.reserve(503);";
	cout<<"\t\tfor (int c=0; image_db[c].width; c++)\n";
	cout<<"\t\t\timages.insert(image_db[c].id, &image_db[c]);\n";
	cout<<"\t}\n";
	cout<<"\tstatic KeramikImageDb* instance;\n";
	cout<<"\tQHash<int, const KeramikEmbedImage*> images;\n";
	cout<<"};\n\n";
	cout<<"KeramikImageDb* KeramikImageDb::instance = 0;\n\n";

	cout<<"const KeramikEmbedImage* KeramikGetDbImage(int id)\n";
	cout<<"{\n";
	cout<<"\treturn KeramikImageDb::getInstance()->getImage(id);\n";
	cout<<"}\n\n";

	cout<<"void KeramikDbCleanup()\n";
	cout<<"{\n";
	cout<<"\t\tKeramikImageDb::release();\n";
	cout<<"}\n";
	cout.flush();


	QFile file("keramikrc.h");
	file.open(QIODevice::WriteOnly);
	QTextStream ts( &file);
	ts<<"#ifndef KERAMIK_RC_H\n";
	ts<<"#define KERAMIK_RC_H\n";

	ts<<"enum KeramikWidget {\n";
	for (QMap<QString, int>::iterator i = assignID.begin(); i != assignID.end(); ++i)
	{
		QString name = "keramik_"+i.key();
		name.replace("-","_");
		ts<<"\t"<<name<<" = "<<i.value()<<",\n";
	}
	ts<<"\tkeramik_last\n";
	ts<<"};\n";

	ts<<"#endif\n";
	file.close();

	return 0;
}

// vim: ts=4 sw=4 noet
