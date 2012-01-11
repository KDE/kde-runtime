/*
	Copyright (C) 2010 by BetterInbox <contact@betterinbox.com>
	Original author: Gregory Schlomoff <greg@betterinbox.com>

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#ifndef DECLARATIVEDRAGDROPEVENT_H
#define DECLARATIVEDRAGDROPEVENT_H

#include <QObject>
#include <QGraphicsSceneDragDropEvent>
#include "DeclarativeMimeData.h"

class DeclarativeDragDropEvent : public QObject
{
    Q_OBJECT

    /**
     * The mouse X position of the event relative to the view that sent the event.
     */
    Q_PROPERTY(int x READ x)

    /**
     * The mouse Y position of the event relative to the view that sent the event.
     */
    Q_PROPERTY(int y READ y)

    /**
     * The pressed mouse buttons.
     * A combination of:
     *  Qt.NoButton    The button state does not refer to any button (see QMouseEvent::button()).
     *  Qt.LeftButton    The left button is pressed, or an event refers to the left button. (The left button may be the right button on left-handed mice.)
     *  Qt.RightButton    The right button.
     *  Qt.MidButton    The middle button.
     *  Qt.MiddleButton  MidButton  The middle button.
     *  Qt.XButton1    The first X button.
     *  Qt.XButton2    The second X button.
     */
    Q_PROPERTY(int buttons READ buttons)

    /**
     * Pressed keyboard modifiers, a combination of:
     *  Qt.NoModifier    No modifier key is pressed.
     *  Qt.ShiftModifier    A Shift key on the keyboard is pressed.
     *  Qt.ControlModifier    A Ctrl key on the keyboard is pressed.
     *  Qt.AltModifier    An Alt key on the keyboard is pressed.
     *  Qt.MetaModifier    A Meta key on the keyboard is pressed.
     *  Qt.KeypadModifier    A keypad button is pressed.
     *  Qt.GroupSwitchModifier    X11 only. A Mode_switch key on the keyboard is pressed.
     */
    Q_PROPERTY(int modifiers READ modifiers)

    /**
     * The mime data of this operation
     * @see DeclarativeMimeData
     */
    Q_PROPERTY(DeclarativeMimeData* mimeData READ mimeData)

    /**
     * The possible different kind of action that can be done in the drop, is a combination of:
     *  Qt.CopyAction  0x1  Copy the data to the target.
     *  Qt.MoveAction  0x2  Move the data from the source to the target.
     *  Qt.LinkAction  0x4  Create a link from the source to the target.
     *  Qt.ActionMask  0xff   
     *  Qt.IgnoreAction  0x0  Ignore the action (do nothing with the data).
     *  Qt.TargetMoveAction  0x8002  On Windows, this value is used when the ownership of the D&D data should be taken over by the target application, i.e., the source application should not delete the data.
     *  On X11 this value is used to do a move.
     *  TargetMoveAction is not used on the Mac.
     */
    Q_PROPERTY(Qt::DropActions possibleActions READ possibleActions)

    /**
     * Default action
     * @see possibleActions
     */
    Q_PROPERTY(Qt::DropAction proposedAction READ proposedAction)

public:

    DeclarativeDragDropEvent(QGraphicsSceneDragDropEvent* e, QObject* parent = 0);

    int x() const { return m_x; }
    int y() const { return m_y; }
    int buttons() const { return m_buttons; }
    int modifiers() const { return m_modifiers; }
    DeclarativeMimeData* mimeData() { return &m_data; }
    Qt::DropAction proposedAction() const { return m_event->proposedAction(); }
    Qt::DropActions possibleActions() const { return m_event->possibleActions(); }

public Q_SLOTS:
    void accept(int action);

private:
    int m_x;
    int m_y;
    Qt::MouseButtons m_buttons;
    Qt::KeyboardModifiers m_modifiers;
    DeclarativeMimeData m_data;
    QGraphicsSceneDragDropEvent* m_event;
};

#endif // DECLARATIVEDRAGDROPEVENT_H
