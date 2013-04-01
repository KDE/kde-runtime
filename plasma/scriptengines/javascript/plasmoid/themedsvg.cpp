/*
 *   Copyright 2007-2008,2010 Richard J. Moore <rich@kde.org>
 *   Copyright 2009 Aaron J. Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "themedsvg.h"

#include <KDebug>

#include "appletinterface.h"

ThemedSvg::ThemedSvg(QObject *parent)
    : Plasma::Svg(parent)
{
}

void ThemedSvg::setThemedImagePath(const QString &path)
{
    setImagePath(findSvg(engine(), path));
}

static QString findLocalSvgFile(AppletInterface *interface, const QString &dir, const QString &file)
{
    QString path = interface->file(dir, file % QLatin1Literal(".svg"));
    if (path.isEmpty()) {
        path = interface->file(dir, file % QLatin1Literal(".svgz"));
    }
    return path;
}

QString ThemedSvg::findSvg(QScriptEngine *engine, const QString &file)
{
    AppletInterface *interface = AppletInterface::extract(engine);
    if (!interface) {
        return QString();
    }

    QString path = findLocalSvgFile(interface, "images", file);
    if (!path.isEmpty()) {
        return path;
    }

    path = Plasma::Theme::defaultTheme()->imagePath(file);
    if (!path.isEmpty()) {
        return path;
    }

    // FIXME: this isn't particularly helpful, as we can't look in the fallback themes
    QString themeName = Plasma::Theme::defaultTheme()->themeName();
    path = findLocalSvgFile(interface, "theme", themeName % QLatin1Literal("/") % file);
    if (!path.isEmpty()) {
        return path;
    }

    path = findLocalSvgFile(interface, "theme", file);
    return path;
}

ThemedFrameSvg::ThemedFrameSvg(QObject *parent)
    : Plasma::FrameSvg(parent)
{
}

void ThemedFrameSvg::setThemedImagePath(const QString &path)
{
    setImagePath(ThemedSvg::findSvg(engine(), path));
}

#include "themedsvg.moc"

