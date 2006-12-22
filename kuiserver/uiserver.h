/* This file is part of the KDE libraries
   Copyright (C) 2000 Matej Koss <koss@miesto.sk>
                      David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef __kio_uiserver_h__
#define __kio_uiserver_h__

#include <q3intdict.h>
#include <qdatetime.h>
#include <qtimer.h>

#include <kio/global.h>
#include <kio/authinfo.h>
#include <kurl.h>
#include <kmainwindow.h>
#include <k3listview.h>
#include <ksslcertdlg.h>
#include <ktoolbar.h>

class ListProgress;
class KSqueezedTextLabel;
class ProgressItem;
class UIServer;

namespace KIO {
  class Job;
  class DefaultProgress;
}


struct ListProgressColumnConfig
{
   QString title;
   int index;
   int width;
   bool enabled;
};

/**
* List view in the UIServer.
* @internal
*/
class ListProgress : public K3ListView {

  Q_OBJECT

public:

  ListProgress (QWidget *parent = 0 );

  virtual ~ListProgress();

  /**
   * Field constants
   */
  enum ListProgressFields {
    TB_OPERATION = 0,
    TB_LOCAL_FILENAME = 1,
    TB_RESUME = 2,
    TB_COUNT = 3,     //lv_count
    TB_PROGRESS = 4,  // lv_progress
    TB_TOTAL = 5,
    TB_SPEED = 6,
    TB_REMAINING_TIME = 7,
    TB_ADDRESS = 8,
    TB_MAX = 9
  };

  friend class ProgressItem;
  friend class UIServer;
protected Q_SLOTS:
  void columnWidthChanged(int column);
protected:

  void writeSettings();
  void readSettings();
  void applySettings();
  void createColumns();

  bool m_showHeader;
  bool m_fixedColumnWidths;
  ListProgressColumnConfig m_lpcc[TB_MAX];
  //hack, alexxx
  KSqueezedTextLabel *m_squeezer;
};

/**
* One item in the ListProgress
* @internal
*/
class ProgressItem : public QObject, public Q3ListViewItem {

  Q_OBJECT

public:
  ProgressItem( ListProgress* view, Q3ListViewItem *after, QByteArray app_id, int job_id,
                bool showDefault = true );
  ~ProgressItem();

  QByteArray appId() { return m_sAppId; }
  int jobId() { return m_iJobId; }

    bool keepOpen() const;
  void finished();

  void setVisible( bool visible );
  void setDefaultProgressVisible( bool visible );
  bool isVisible() const { return m_visible; }

  void setTotalSize( KIO::filesize_t bytes );
  void setTotalFiles( unsigned long files );
  void setTotalDirs( unsigned long dirs );

  void setProcessedSize( KIO::filesize_t size );
  void setProcessedFiles( unsigned long files );
  void setProcessedDirs( unsigned long dirs );

  void setPercent( unsigned long percent );
  void setSpeed( unsigned long bytes_per_second );
  void setInfoMessage( const QString & msg );

  void setCopying( const KUrl& from, const KUrl& to );
  void setMoving( const KUrl& from, const KUrl& to );
  void setDeleting( const KUrl& url );
  void setTransferring( const KUrl& url );
  void setCreatingDir( const KUrl& dir );
  void setStating( const KUrl& url );
  void setMounting( const QString & dev, const QString & point );
  void setUnmounting( const QString & point );

  void setCanResume( KIO::filesize_t offset );

  KIO::filesize_t totalSize() { return m_iTotalSize; }
  unsigned long totalFiles() { return m_iTotalFiles; }
  KIO::filesize_t processedSize() { return m_iProcessedSize; }
  unsigned long processedFiles() { return m_iProcessedFiles; }
  unsigned long speed() { return m_iSpeed; }
  unsigned int remainingSeconds() { return m_remainingSeconds; }

  const QString& fullLengthAddress() const {return m_fullLengthAddress;}
  void setText(ListProgress::ListProgressFields field, const QString& text);
public Q_SLOTS:
  void slotShowDefaultProgress();
  void slotToggleDefaultProgress();

protected Q_SLOTS:
  void slotCanceled();
  void slotSuspend();
  void slotResume();

Q_SIGNALS:
  void jobCanceled( ProgressItem* );
  void jobSuspended( ProgressItem* );
  void jobResumed( ProgressItem* );

protected:
  void updateVisibility();

  // ids that uniquely identify this progress item
  QByteArray m_sAppId;
  int m_iJobId;

  // whether shown or not (it is hidden if a rename dialog pops up for the same job)
  bool m_visible;
  bool m_defaultProgressVisible;

  // parent listview
  ListProgress *listProgress;

  // associated default progress dialog
  KIO::DefaultProgress *defaultProgress;

  // we store these values for calculation of totals ( for statusbar )
  KIO::filesize_t m_iTotalSize;
  unsigned long m_iTotalFiles;
  KIO::filesize_t m_iProcessedSize;
  unsigned long m_iProcessedFiles;
  unsigned long m_iSpeed;
  int m_remainingSeconds;
  QTimer m_showTimer;
  QString m_fullLengthAddress;
};

class QResizeEvent;
class QHideEvent;
class QShowEvent;
class ProgressConfigDialog;
class QMenu;
class KSystemTrayIcon;

/**
 * It's purpose is to show progress of IO operations.
 * There is only one instance of this window for all jobs.
 *
 * All IO operations ( jobs ) are displayed in this window, one line per operation.
 * User can cancel operations with Cancel button on toolbar.
 *
 * Double clicking an item in the list opens a small download window ( DefaultProgress ).
 *
 * @short Graphical server for progress information with an optional all-in-one progress window.
 * @author David Faure <faure@kde.org>
 * @author Matej Koss <koss@miesto.sk>
 *
 * @internal
 */
class UIServer : public KMainWindow {

  Q_OBJECT

  UIServer();
  virtual ~UIServer();

public:
   static UIServer* createInstance();

  /**
   * Signal a new job
   * @param appId the DCOP application id of the job's parent application
   * @see KIO::Observer::newJob
   * @param showProgress whether to popup the progress for the job.
   *   Usually true, but may be false when we use kio_uiserver for
   *   other things, like SSL dialogs.
   * @return the job id
   */
  int newJob( const QString &appId, bool showProgress );

  void jobFinished( int id );

  void totalSize( int id, KIO::filesize_t size );
  void totalFiles( int id, unsigned long files );
  void totalDirs( int id, unsigned long dirs );

  void processedSize( int id, KIO::filesize_t bytes );
  void processedFiles( int id, unsigned long files );
  void processedDirs( int id, unsigned long dirs );

  void percent( int id, unsigned long ipercent );
  void speed( int id, unsigned long bytes_per_second );
  void infoMessage( int id, const QString & msg );

  void copying( int id, KUrl from, KUrl to );
  void moving( int id, KUrl from, KUrl to );
  void deleting( int id, KUrl url );
  void transferring( int id, KUrl url );
  void creatingDir( int id, KUrl dir );
  void stating( int id, KUrl url );

  void mounting( int id, QString dev, QString point );
  void unmounting( int id, QString point );

  void canResume( int id, KIO::filesize_t offset );


  /**
   * Popup a message box.
   * @param type type of message box: QuestionYesNo, WarningYesNo, WarningContinueCancel...
   *   This enum is defined in slavebase.h, it currently is:
   *   QuestionYesNo = 1, WarningYesNo = 2, WarningContinueCancel = 3,
   *   WarningYesNoCancel = 4, Information = 5, SSLMessageBox = 6
   * @param text Message string. May contain newlines.
   * @param caption Message box title.
   * @param buttonYes The text for the first button.
   *                  The default is i18n("&Yes").
   * @param buttonNo  The text for the second button.
   *                  The default is i18n("&No").
   * Note: for ContinueCancel, buttonYes is the continue button and buttonNo is unused.
   *       and for Information, none is used.
   * @return a button code, as defined in KMessageBox, or 0 on communication error.
   */
  int messageBox( int id, int type, const QString &text, const QString &caption,
                  const QString &buttonYes, const QString &buttonNo );

  /**
   * @deprecated (it blocks other apps).
   * Use KIO::open_RenameDlg instead.
   * To be removed in KDE 4.0.
   */
  QByteArray open_RenameDlg64( int id,
                             const QString & caption,
                             const QString& src, const QString & dest,
                             int /* KIO::RenameDlg_Mode */ mode,
                             KIO::filesize_t sizeSrc,
                             KIO::filesize_t sizeDest,
                             unsigned long /* time_t */ ctimeSrc,
                             unsigned long /* time_t */ ctimeDest,
                             unsigned long /* time_t */ mtimeSrc,
                             unsigned long /* time_t */ mtimeDest
                             );
  /**
   * @deprecated (it blocks other apps).
   * Use KIO::open_RenameDlg instead.
   * To be removed in KDE 4.0.
   */
  QByteArray open_RenameDlg( int id,
                             const QString & caption,
                             const QString& src, const QString & dest,
                             int /* KIO::RenameDlg_Mode */ mode,
                             unsigned long sizeSrc,
                             unsigned long sizeDest,
                             unsigned long /* time_t */ ctimeSrc,
                             unsigned long /* time_t */ ctimeDest,
                             unsigned long /* time_t */ mtimeSrc,
                             unsigned long /* time_t */ mtimeDest
                             );

  /**
   * @deprecated (it blocks other apps).
   * Use KIO::open_SkipDlg instead.
   * To be removed in KDE 4.0.
   */
  int open_SkipDlg( int id,
                    int /*bool*/ multi,
                    const QString & error_text );

  /**
   * Switch to or from list mode - called by the kcontrol module
   */
  void setListMode( bool list );

  /**
   * Hide or show a job. Typically, we hide a job while a "skip" or "rename" dialog
   * is being shown for this job. This prevents killing it from the uiserver.
   */
  void setJobVisible( int id, bool visible );

  /**
   * Show a SSL Information Dialog
   */
  void showSSLInfoDialog(const QString &url, const KIO::MetaData &data, int mainwindow);

  /**
   * @deprecated
   */
  void showSSLInfoDialog(const QString &url, const KIO::MetaData &data);

  /*
   * Show an SSL Certificate Selection Dialog
   */
  KSSLCertDlgRet showSSLCertDialog(const QString& host, const QStringList& certList, int mainwindow);

  /*
   * @deprecated
   */
  KSSLCertDlgRet showSSLCertDialog(const QString& host, const QStringList& certList);

public Q_SLOTS:
  void slotConfigure();
  void slotRemoveSystemTrayIcon();
protected Q_SLOTS:

  void slotUpdate();
  void slotQuit();

  void slotCancelCurrent();

  void slotToggleDefaultProgress( Q3ListViewItem * );
  void slotSelection();

  void slotJobCanceled( ProgressItem * );
  void slotJobSuspended( ProgressItem* );
  void slotJobResumed( ProgressItem* );
  void slotApplyConfig();
  void slotShowContextMenu(K3ListView*, Q3ListViewItem *item, const QPoint& pos);

protected:

  ProgressItem* findItem( int id );

  virtual void resizeEvent(QResizeEvent* e);
  virtual bool queryClose();

  void setItemVisible( ProgressItem * item, bool visible );

  QTimer* updateTimer;
  ListProgress* listProgress;

  Qt::ToolBarArea toolbarPos;
  QString properties;

  void applySettings();
  void readSettings();
  void writeSettings();
private:

  void callObserver( const QString &observerAppId, int progressId, const char* method );

  int m_initWidth;
  int m_initHeight;
  QAction *m_cancelAction;
  bool m_bShowList;
  bool m_showStatusBar;
  bool m_showToolBar;
  bool m_keepListOpen;
  bool m_showSystemTray;
  bool m_shuttingDown;

  // true if there's a new job that hasn't been shown yet.
  bool m_bUpdateNewJob;
  ProgressConfigDialog *m_configDialog;
  QMenu* m_contextMenu;
  KSystemTrayIcon *m_systemTray;

  QAction *m_toolCancel, *m_toolConfigure;

  static int s_jobId;
  friend class no_bogus_warning_from_gcc;
};

// -*- mode: c++; c-basic-offset: 2 -*-
#endif
