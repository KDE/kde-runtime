/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "listjob.cpp"
#include "itemjob.cpp"
#include "parser.cpp"

#include "activity.h"
#include "activityparser.h"
#include "category.h"
#include "categoryparser.h"
#include "content.h"
#include "contentparser.h"
#include "downloaditemparser.h"
#include "event.h"
#include "eventparser.h"
#include "folder.h"
#include "folderparser.h"
#include "message.h"
#include "messageparser.h"
#include "person.h"
#include "personparser.h"
#include "knowledgebaseentry.h"
#include "knowledgebaseentryparser.h"

using namespace Attica;

template class ListJob<Activity>;
template class ListJob<Category>;
template class ListJob<Content>;
template class ListJob<Event>;
template class ListJob<Folder>;
template class ListJob<Message>;
template class ListJob<Person>;
template class ListJob<KnowledgeBaseEntry>;
template class ListJob<DownloadItem>;

template class ItemJob<Content>;
template class ItemJob<Event>;
template class ItemJob<Message>;
template class ItemJob<KnowledgeBaseEntry>;
template class ItemJob<DownloadItem>;
template class ItemJob<Person>;
template class ItemPostJob<Content>;

template class Parser<Activity>;
template class Parser<Category>;
template class Parser<Content>;
template class Parser<Event>;
template class Parser<Folder>;
template class Parser<KnowledgeBaseEntry>;
template class Parser<Message>;
template class Parser<Person>;
template class Parser<DownloadItem>;
