/***************************************************************************
 *   Copyright 2010 Marco Martin <mart@kde.org>                            *
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
#ifndef FRAMESVGITEM_P
#define FRAMESVGITEM_P

#include <QDeclarativeItem>

#include <Plasma/FrameSvg>

namespace Plasma {

    class FrameSvg;

class FrameSvgItemMargins : public QObject
{
    Q_OBJECT

    /**
     * width in pixels of the left margin
     */
    Q_PROPERTY(qreal left READ left NOTIFY marginsChanged)

    /**
     * height in pixels of the top margin
     */
    Q_PROPERTY(qreal top READ top NOTIFY marginsChanged)

    /**
     * width in pixels of the right margin
     */
    Q_PROPERTY(qreal right READ right NOTIFY marginsChanged)

    /**
     * height in pixels of the bottom margin
     */
    Q_PROPERTY(qreal bottom READ bottom NOTIFY marginsChanged)

public:
    FrameSvgItemMargins(Plasma::FrameSvg *frameSvg, QObject *parent = 0);

    qreal left() const;
    qreal top() const;
    qreal right() const;
    qreal bottom() const;

public Q_SLOTS:
    void update();

Q_SIGNALS:
    void marginsChanged();

private:
    FrameSvg *m_frameSvg;
};

class FrameSvgItem : public QDeclarativeItem
{
    Q_OBJECT

    /**
     * Theme relative path of the svg, like "widgets/background"
     */
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    /**
     * prefix for the 9 piece svg, like "pushed" or "normal" for the button
     * see http://techbase.kde.org/Development/Tutorials/Plasma/ThemeDetails
     * for a list of paths and prefixes
     */
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)

    /**
     * The margins of the frame, read only
     * @see FrameSvgItemMargins
     */
    Q_PROPERTY(QObject *margins READ margins CONSTANT)

    Q_FLAGS(Plasma::FrameSvg::EnabledBorders)
    /**
     * The borders that will be rendered, it's a flag combination of:
     *  NoBorder
     *  TopBorder
     *  BottomBorder
     *  LeftBorder
     *  RightBorder
     */
    Q_PROPERTY(Plasma::FrameSvg::EnabledBorders enabledBorders READ enabledBorders WRITE setEnabledBorders NOTIFY enabledBordersChanged)

public:
    FrameSvgItem(QDeclarativeItem *parent=0);
    ~FrameSvgItem();

    void setImagePath(const QString &path);
    QString imagePath() const;

    void setPrefix(const QString &prefix);
    QString prefix() const;

    void setEnabledBorders(const Plasma::FrameSvg::EnabledBorders borders);
    Plasma::FrameSvg::EnabledBorders enabledBorders() const;

    FrameSvgItemMargins *margins() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void geometryChanged(const QRectF &newGeometry,
                              const QRectF &oldGeometry);

signals:
    void imagePathChanged();
    void prefixChanged();
    void enabledBordersChanged();

private Q_SLOTS:
    void doUpdate();

private:
    Plasma::FrameSvg *m_frameSvg;
    FrameSvgItemMargins *m_margins;
    QString m_prefix;
};

}

#endif
