/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

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

#ifndef __moduleinfo_h__
#define __moduleinfo_h__

#include <qobject.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qstringlist.h>


#include <kservice.h>


class ModuleInfo : public QObject
{
  Q_OBJECT

public:

  ModuleInfo(QString desktopFile);
  ~ModuleInfo();

  const QString fileName() const;
  const QStringList &groups() const;
  const QStringList &keywords() const;
  QString name() const;
  KService::Ptr service() const;
  QString comment() const;
  QString icon() const;
  QString docPath() const;
  QString library() const;
  QString handle() const;
  bool isDirectory() const;
  bool needsRootPrivileges() const;
  bool isHiddenByDefault() const;

  QCString moduleId() const;

protected:

  void setGroups(const QStringList &groups) { _groups = groups; };
  void setKeywords(const QStringList &k) { _keywords = k; };
  void setName(const QString &name) { _name = name; };
  void setComment(const QString &comment) { _comment = comment; };
  void setIcon(const QString &icon) { _icon = icon; };
  void setDirectory(bool dir) { _directory = dir; };
  void setLibrary(const QString &lib) { _lib = lib; };
  void setHandle(const QString &handle) { _handle = handle; };
  void setNeedsRootPrivileges(bool needsRootPrivileges) { _needsRootPrivileges = needsRootPrivileges; };
  void setIsHiddenByDefault(bool isHiddenByDefault) { _isHiddenByDefault = isHiddenByDefault; };
  void setDocPath(const QString &p) { _doc = p; };
  void loadAll();

private:

  QStringList _groups, _keywords;
  QString     _name, _icon, _lib, _handle, _fileName, _doc, _comment;
  bool        _directory, _needsRootPrivileges, _isHiddenByDefault;
  bool        _allLoaded;

  KService::Ptr _service;

};

#endif
