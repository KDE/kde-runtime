/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <kdesktopfile.h>
#include <kdebug.h>

#include "moduleinfo.h"
#include "moduleinfo.moc"

KCModuleInfo::KCModuleInfo(const QString& desktopFile, const QString& baseGroup)
  : _fileName(desktopFile)
{
  _allLoaded = false;

  //kdDebug(1208) << "desktopFile = " << desktopFile << endl;
  _service = KService::serviceByDesktopPath(desktopFile);
  Q_ASSERT(_service != 0L);

  // set the modules simple attributes
  setName(_service->name());
  setComment(_service->comment());
  setIcon(_service->icon());

  // library and factory
  setLibrary(_service->library());

  // get the keyword list
  setKeywords(_service->keywords());

  // try to find out the modules groups
  QString group = desktopFile;

  int pos = group.find(baseGroup);
  if (pos >= 0)
     group = group.mid(pos+baseGroup.length());
  pos = group.findRev('/');
  if (pos >= 0)
     group = group.left(pos);
  else
     group = QString::null;

  QStringList groups = QStringList::split('/', group);
  setGroups(groups);
}

KCModuleInfo::~KCModuleInfo() { }

void
KCModuleInfo::loadAll() 
{
  _allLoaded = true;

  KDesktopFile desktop(_fileName);

  // library and factory
  setHandle(desktop.readEntry("X-KDE-FactoryName"));

  // does the module need super user privileges?
  setNeedsRootPrivileges(desktop.readBoolEntry("X-KDE-RootOnly", false));

  // does the module need to be shown to root only?
  // Deprecated !
  setIsHiddenByDefault(desktop.readBoolEntry("X-KDE-IsHiddenByDefault", false));

  // get the documentation path
  setDocPath(desktop.readEntry("DocPath"));
}

QCString 
KCModuleInfo::moduleId() const
{
  if (!_allLoaded) const_cast<KCModuleInfo*>(this)->loadAll();

  QString res;

  QStringList::ConstIterator it;
  for (it = _groups.begin(); it != _groups.end(); ++it)
    res.append(*it+"-");
  res.append(moduleName());

  return res.ascii();
}

QString
KCModuleInfo::docPath() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _doc;
};

QString
KCModuleInfo::handle() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  if (_handle.isEmpty())
     return _lib;

  return _handle;
};

bool
KCModuleInfo::needsRootPrivileges() const
{
  if (!_allLoaded) 
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _needsRootPrivileges;
};

bool
KCModuleInfo::isHiddenByDefault() const
{
  if (!_allLoaded)
    const_cast<KCModuleInfo*>(this)->loadAll();

  return _isHiddenByDefault;
};

// vim: ts=2 sw=2 et
