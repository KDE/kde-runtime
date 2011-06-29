/* This file is part of the KDE libraries
    Copyright (C) 2001,2002 Ellis Whitehead <ellis@kde.org>

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

#include "kglobalaccel_x11.h"

#include <QtGui/QWidgetList>

#include "kaction.h"
#include "globalshortcutsregistry.h"
#include "kkeyserver_x11.h"
#include "kkeysequencewidget.h"

#include <kapplication.h>
#include <kdebug.h>

#include <QtCore/QRegExp>
#include <QtGui/QWidget>
#include <QtCore/QMetaClassInfo>
#include <QtGui/QMenu>

#include <kxerrorhandler.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <fixx11h.h>

extern "C" {
  static int XGrabErrorHandler( Display *, XErrorEvent *e ) {
	if ( e->error_code != BadAccess ) {
	    kWarning() << "grabKey: got X error " << e->type << " instead of BadAccess\n";
	}
	return 1;
  }
}

// g_keyModMaskXAccel
//	mask of modifiers which can be used in shortcuts
//	(meta, alt, ctrl, shift)
// g_keyModMaskXOnOrOff
//	mask of modifiers where we don't care whether they are on or off
//	(caps lock, num lock, scroll lock)
static uint g_keyModMaskXAccel = 0;
static uint g_keyModMaskXOnOrOff = 0;

static void calculateGrabMasks()
{
	g_keyModMaskXAccel = KKeyServer::accelModMaskX();
	g_keyModMaskXOnOrOff =
			KKeyServer::modXLock() |
			KKeyServer::modXNumLock() |
			KKeyServer::modXScrollLock() |
			KKeyServer::modXModeSwitch();
	//kDebug() << "g_keyModMaskXAccel = " << g_keyModMaskXAccel
	//	<< "g_keyModMaskXOnOrOff = " << g_keyModMaskXOnOrOff << endl;
}

//----------------------------------------------------

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry *owner)
	: m_owner(owner)
{
	calculateGrabMasks();
}

bool KGlobalAccelImpl::grabKey( int keyQt, bool grab )
{
	if( !keyQt ) {
        kDebug() << "Tried to grab key with null code.";
		return false;
	}

	int keyCodeX;
	uint keyModX;
	uint keySymX;

	// Resolve the modifier
	if( !KKeyServer::keyQtToModX(keyQt, &keyModX) ) {
		kDebug() << "keyQt (0x" << hex << keyQt << ") failed to resolve to x11 modifier";
		return false;
	}

	// Resolve the X symbol
	if( !KKeyServer::keyQtToSymX(keyQt, (int *)&keySymX) ) {
		kDebug() << "keyQt (0x" << hex << keyQt << ") failed to resolve to x11 keycode";
		return false;
	}

	keyCodeX = XKeysymToKeycode( QX11Info::display(), keySymX );

	// Check if shift needs to be added to the grab since KKeySequenceWidget
	// can remove shift for some keys. (all the %&* and such)
	if( !(keyQt & Qt::SHIFT) &&
	    !KKeySequenceWidget::isShiftAsModifierAllowed( keyQt ) &&
	    keySymX != XKeycodeToKeysym( QX11Info::display(), keyCodeX, 0 ) &&
	    keySymX == XKeycodeToKeysym( QX11Info::display(), keyCodeX, 1 ) )
	{
		kDebug() << "adding shift to the grab";
		keyModX |= KKeyServer::modXShift();
	}

	keyModX &= g_keyModMaskXAccel; // Get rid of any non-relevant bits in mod

	if( !keyCodeX ) {
		kDebug() << "keyQt (0x" << hex << keyQt << ") was resolved to x11 keycode 0";
		return false;
	}

	KXErrorHandler handler( XGrabErrorHandler );

	// We'll have to grab 8 key modifier combinations in order to cover all
	//  combinations of CapsLock, NumLock, ScrollLock.
	// Does anyone with more X-savvy know how to set a mask on QX11Info::appRootWindow so that
	//  the irrelevant bits are always ignored and we can just make one XGrabKey
	//  call per accelerator? -- ellis
#ifndef NDEBUG
	QString sDebug = QString("\tcode: 0x%1 state: 0x%2 | ").arg(keyCodeX,0,16).arg(keyModX,0,16);
#endif
	uint keyModMaskX = ~g_keyModMaskXOnOrOff;
	for( uint irrelevantBitsMask = 0; irrelevantBitsMask <= 0xff; irrelevantBitsMask++ ) {
		if( (irrelevantBitsMask & keyModMaskX) == 0 ) {
#ifndef NDEBUG
			sDebug += QString("0x%3, ").arg(irrelevantBitsMask, 0, 16);
#endif
			if( grab )
				XGrabKey( QX11Info::display(), keyCodeX, keyModX | irrelevantBitsMask,
					QX11Info::appRootWindow(), True, GrabModeAsync, GrabModeSync );
			else
				XUngrabKey( QX11Info::display(), keyCodeX, keyModX | irrelevantBitsMask, QX11Info::appRootWindow() );
		}
	}

	bool failed = false;
	if( grab ) {
		failed = handler.error( true ); // sync now
		if( failed ) {
			kDebug() << "grab failed!\n";
			for( uint m = 0; m <= 0xff; m++ ) {
				if(( m & keyModMaskX ) == 0 )
					XUngrabKey( QX11Info::display(), keyCodeX, keyModX | m, QX11Info::appRootWindow() );
			}
		}
	}

	return !failed;
}

bool KGlobalAccelImpl::x11Event( XEvent* event )
{
	switch( event->type ) {

		case MappingNotify:
			kDebug() << "Got XMappingNotify event";
			XRefreshKeyboardMapping(&event->xmapping);
			x11MappingNotify();
			return true;

		case XKeyPress:
			kDebug() << "Got XKeyPress event";
			return x11KeyPress(event);

		default:
			// We get all XEvents. Just ignore them.
			return false;
	}

	Q_ASSERT(false);
	return false;
}

void KGlobalAccelImpl::x11MappingNotify()
{
	// Maybe the X modifier map has been changed.
	// uint oldKeyModMaskXAccel = g_keyModMaskXAccel;
	// uint oldKeyModMaskXOnOrOff = g_keyModMaskXOnOrOff;

	// First ungrab all currently grabbed keys. This is needed because we
	// store the keys as qt keycodes and use KKeyServer to map them to x11 key
	// codes. After calling KKeyServer::initializeMods() they could map to
	// different keycodes.
	m_owner->ungrabKeys();

	KKeyServer::initializeMods();
	calculateGrabMasks();

	m_owner->grabKeys();
}


bool KGlobalAccelImpl::x11KeyPress( const XEvent *pEvent )
{
	if (QWidget::keyboardGrabber() || QApplication::activePopupWidget()) {
		kWarning() << "kglobalacceld should be popup and keyboard grabbing free!";
	}

	// Keyboard needs to be ungrabed after XGrabKey() activates the grab,
	// otherwise it becomes frozen.
	XUngrabKeyboard( QX11Info::display(), CurrentTime );
	XFlush( QX11Info::display()); // avoid X(?) bug

	uchar keyCodeX = pEvent->xkey.keycode;
	uint keyModX = pEvent->xkey.state & (g_keyModMaskXAccel | KKeyServer::MODE_SWITCH);

	KeySym keySym;
	XLookupString( (XKeyEvent*) pEvent, 0, 0, &keySym, 0 );
	uint keySymX = (uint)keySym;

	// If numlock is active and a keypad key is pressed, XOR the SHIFT state.
	//  e.g., KP_4 => Shift+KP_Left, and Shift+KP_4 => KP_Left.
	if( pEvent->xkey.state & KKeyServer::modXNumLock() ) {
		uint sym = XKeycodeToKeysym( QX11Info::display(), keyCodeX, 0 );
		// If this is a keypad key,
		if( sym >= XK_KP_Space && sym <= XK_KP_9 ) {
			switch( sym ) {

				// Leave the following keys unaltered
				// FIXME: The proper solution is to see which keysyms don't change when shifted.
				case XK_KP_Multiply:
				case XK_KP_Add:
				case XK_KP_Subtract:
				case XK_KP_Divide:
					break;

				default:
					keyModX ^= KKeyServer::modXShift();
			}
		}
	}

	int keyCodeQt;
	int keyModQt;
	KKeyServer::symXToKeyQt(keySymX, &keyCodeQt);
	KKeyServer::modXToQt(keyModX, &keyModQt);

	if( keyModQt & Qt::SHIFT && !KKeySequenceWidget::isShiftAsModifierAllowed( keyCodeQt ) ) {
		kDebug() << "removing shift modifier";
		keyModQt &= ~Qt::SHIFT;
	}

	int keyQt = keyCodeQt | keyModQt;

	// All that work for this hey... argh...
	return m_owner->keyPressed(keyQt);
}

void KGlobalAccelImpl::setEnabled( bool enable )
{
	if (enable) {
		kapp->installX11EventFilter( this );
	} else
		kapp->removeX11EventFilter( this );
}


#include "kglobalaccel_x11.moc"
