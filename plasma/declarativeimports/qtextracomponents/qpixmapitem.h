/***************************************************************************
 *   Copyright 2011 Marco Martin <mart@kde.org>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#ifndef QPIXMAPITEM_H
#define QPIXMAPITEM_H

#include <QDeclarativeItem>
#include <QPixmap>

class QPixmapItem : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth)
    Q_PROPERTY(int nativeWidth READ nativeWidth NOTIFY nativeWidthChanged)
    Q_PROPERTY(int nativeHeight READ nativeHeight NOTIFY nativeHeightChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_ENUMS(FillMode)

public:
    enum FillMode {
        Stretch, // the image is scaled to fit
        PreserveAspectFit, // the image is scaled uniformly to fit without cropping
        PreserveAspectCrop, // the image is scaled uniformly to fill, cropping if necessary
        Tile, // the image is duplicated horizontally and vertically
        TileVertically, // the image is stretched horizontally and tiled vertically
        TileHorizontally //the image is stretched vertically and tiled horizontally
    };

    QPixmapItem(QDeclarativeItem *parent=0);
    ~QPixmapItem();

    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;

    void setSmooth(const bool smooth);
    bool smooth() const;

    int nativeWidth() const;
    int nativeHeight() const;

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

Q_SIGNALS:
    void nativeWidthChanged();
    void nativeHeightChanged();
    void fillModeChanged();

private:
    QPixmap m_pixmap;
    bool m_smooth;
    FillMode m_fillMode;
};

#endif
