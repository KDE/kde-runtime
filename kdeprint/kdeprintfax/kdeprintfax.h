/*
 *   kdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef KDEPRINTFAX_H
#define KDEPRINTFAX_H

#include <kxmlguiwindow.h>
#include <kurl.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QLabel>
#include <QDropEvent>

class KListWidget;
class K3ListView;
class QLineEdit;
class QTextEdit;
class FaxCtrl;
class QLabel;
class QDateTimeEdit;
class QComboBox;
class QPushButton;
class Q3ListViewItem;

class KdeprintFax : public KXmlGuiWindow
{
	Q_OBJECT
public:
	struct FaxItem
	{
		QString number;
		QString name;
		QString enterprise;
	};
	typedef QList<FaxItem> FaxItemList;
	typedef QList<FaxItem>::ConstIterator FaxItemListIterator;

	explicit KdeprintFax(QWidget *parent = 0, const char *name = 0);
	~KdeprintFax();

	void addURL(KUrl url);
	void setPhone(QString phone);
	void sendFax( bool quitAfterSend );
	QStringList files();
	int faxCount() const;
	//QString number( int i = 0 ) const;
	//QString name( int i = 0 ) const;
	//QString enterprise( int i = 0 ) const;
	FaxItemList faxList() const;
	QString comment() const;
	QString time() const;
	QString subject() const;

protected Q_SLOTS:
	void slotToggleMenuBar();
	void slotKab();
	void slotAdd();
	void slotRemove();
	void slotFax();
	void slotAbort();
	void slotMessage(const QString&);
	void slotFaxSent(bool);
	void slotViewLog();
	void slotConfigure();
	void slotQuit();
	void slotView();
	void slotTimeComboActivated(int);
	void slotMoveUp();
	void slotMoveDown();
	void slotCurrentChanged();
	void slotFaxSelectionChanged();
	void slotFaxRemove();
	void slotFaxAdd();
	void slotFaxExecuted( Q3ListViewItem* );
        void slotChangeStateFileViewAction(bool _b);
signals:
	void changeStateFileViewAction(bool);

protected:
	void initActions();
	void dragEnterEvent(QDragEnterEvent*);
	void dropEvent(QDropEvent*);
	void updateState();
	bool manualFaxDialog( QString& number, QString& name, QString& enterprise );
	//QListViewItem* faxItem( int i = 0 ) const;

private:
	KListWidget	*m_files;
	K3ListView   *m_numbers;
	QLineEdit	*m_subject;
	QTextEdit	*m_comment;
	FaxCtrl		*m_faxctrl;
	QLabel		*m_msglabel;
	QDateTimeEdit	*m_time;
	QComboBox	*m_timecombo;
	QPushButton *m_upbtn, *m_downbtn;
	QPushButton *m_newbtn, *m_abbtn, *m_delbtn;
	bool m_quitAfterSend;
};

#endif
