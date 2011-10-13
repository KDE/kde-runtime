/*
    Copyright 2011 Sebastian Kügler <sebas@kde.org>
    Copyright 2010 Richard Moore <rich@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WEBCREATOR_H
#define WEBCREATOR_H

#include <QEventLoop>
#include <QObject>
#include <kio/thumbcreator.h>
#include <QTimerEvent>
#include <QUrl>

class KWebPage;
class WebCreatorPrivate;

class WebCreator : public QObject, public ThumbCreator
{
    Q_OBJECT
public:
    WebCreator();
    virtual ~WebCreator();
    virtual bool create(const QString &path, int width, int height, QImage &img);
    virtual Flags flags() const;

protected:
    virtual void timerEvent(QTimerEvent *);

private Q_SLOTS:
    void completed(bool);

private:
    //WebCreatorPrivate* d;
    KWebPage *m_page;
    //QImage thumbnail;
    //QSize size;
    QUrl m_url;
    QEventLoop m_eventLoop;
    bool m_failed;

};

#endif
