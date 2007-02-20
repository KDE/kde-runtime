/*
  Copyright (c) 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

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

#include <qfileinfo.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qsettings.h>
#include <qcoreapplication.h>

#include <kcomponentdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>

//static const char desc[] = I18N_NOOP("KDE Tool to build a cache list of all pixmap themes installed");
//static const char ver[] = "0.9.1";

int main(int argc, char **argv)
{
    KComponentData componentData("kinstalltheme"); // TODO use KAboutData
    QCoreApplication qapp(argc, argv);

    KGlobal::dirs()->addResourceType("themercs", KGlobal::dirs()->kde_default("data")+QString("kstyle/themes"));
    QStringList themercs = KGlobal::dirs()->findAllResources("themercs","*.themerc");

    QMap <QString, QString> themes; //Name->file mapping..

    for (QStringList::iterator i = themercs.begin(); i!=themercs.end(); ++i)
    {
        QString file=*i;
        KConfig config(file);
        QString name = QFileInfo(file).baseName(); //This is nice and static...
        //So we don't have to worry about our key changing when the language does.

        KConfigGroup cg(&config, "KDE" );

        if (cg.readEntry( "widgetStyle" ) == "basicstyle.la")
        {
            //OK, emit a style entry...
            if (!themes.contains(name)) //Only add first occurrence, i.e. user local one.
                themes[name] = file;
        }
    }

    KConfig cache( KGlobal::dirs()->saveLocation("config")+"kthemestylerc");

#if 0
//Doesn't seem to work with present Qt..
	QStringList existing = cache.subkeyList("/kthemestyle");
	for (QStringList::iterator i = existing.begin(); i != existing.end(); i++)
	{
		cout<<"Have:"<<(*i).toLatin1().constData()<<"\n";
		cache.removeEntry("/ktmthestyle"+(*i));
	}
#endif

    QStringList themeNames; //A list of names, each occurring once - the keys of the themes map..

    for (QMap<QString, QString>::Iterator  i = themes.begin(); i!=themes.end(); ++i)
    {
        KConfigGroup cg(&cache, i.key().toLower());
        cg.writePathEntry("file",QFileInfo(i.value()).fileName());
        themeNames.push_back(i.key());
    }

    cache.group("General").writeEntry("themes", themeNames.join("^e")+"^e");

    return 0;
}
