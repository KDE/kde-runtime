/*
   Copyright (C) 2010 by Jacopo De Simoi <wilderkde@gmail.com>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#ifndef KSOLIDNOTIFY_H
#define KSOLIDNOTIFY_H

#include "knotify.h"

#include <solid/solidnamespace.h>

namespace Solid
{
	class Device;
}

/**
 * @brief Class which triggers solid notifications
 *
 * This is an internal class which listens to solid errors and route them via dbus to an
 * appropriate visualization (e.g. the plasma device notifier applet); if such visualization is not available
 * errors are shown via regular notifications
 *
 * @author Jacopo De Simoi <wilderkde at gmail.com>
*/

class KSolidNotify : public QObject
{ Q_OBJECT

	public:
		KSolidNotify(KNotify *parent);

	protected Q_SLOTS:
		void onDeviceAdded(const QString &udi);
		void onDeviceRemoved(const QString &udi);

	private slots:

		void storageEjectDone(Solid::ErrorType error, QVariant errorData, const QString & udi);
		void storageTeardownDone(Solid::ErrorType error, QVariant errorData, const QString & udi);
		void storageSetupDone(Solid::ErrorType error, QVariant errorData, const QString & udi);

		void slotServiceOwnerChanged(const QString &, const QString &, const QString &);

	private:
		void connectSignals(Solid::Device* device);
		bool isSafelyRemovable(const QString &udi);
		void notifySolidEvent(QString event, Solid::ErrorType error, QVariant errorData, const QString & udi, const QString & errorMessage);

		KNotify* m_kNotify;
		QHash<QString, Solid::Device> m_devices;
		bool m_dbusServiceExists;
};
#endif
