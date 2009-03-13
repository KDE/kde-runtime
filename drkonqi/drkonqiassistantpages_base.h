/*******************************************************************
* drkonqiassistantpages_base.h
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#ifndef DRKONQIASSISTANTPAGES__H
#define DRKONQIASSISTANTPAGES__H

#include <QtGui/QWidget>

#include "getbacktracewidget.h"
#include "crashinfo.h"

class KPushButton;
class QLabel;
class KTextBrowser;
class QCheckBox;

//BASE interface which implements some signals, and aboutTo + setBusy/setIdle functions
class DrKonqiAssistantPage: public QWidget
{
    Q_OBJECT
    
    public:
        DrKonqiAssistantPage()
        {
            isBusy = false;
        }
    
        //aboutToShow may load the widget data if empty
        virtual void aboutToShow() {}
        //aboutToHide may save the widget data to crashInfo, to continue
        virtual void aboutToHide() {}
        
        virtual bool isComplete() { return true; }
        
    public Q_SLOTS:
        
        //Set the page as busy and doesn't allow to change.
        void setBusy()
        {
            if( !isBusy )
            {
                isBusy = true;
                emitCompleteChanged(); //Probably
            }
        }
        
        void setIdle()
        {
            isBusy = false;
            emitCompleteChanged(); //Probably
        }
        
        void emitCompleteChanged()
        {
            emit completeChanged( this, isComplete() );
        }
        
    Q_SIGNALS:
        void completeChanged( DrKonqiAssistantPage*, bool );
        
    private:
        bool isBusy;

};

//Introduction assistant page --------------
class IntroductionPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        IntroductionPage();
};

//Backtrace Page ---------------------------
class CrashInformationPage: public DrKonqiAssistantPage
{

    Q_OBJECT
    
    public:
        CrashInformationPage( CrashInfo * );
        
        void aboutToShow(); 
        bool isComplete();
        
    private:
        GetBacktraceWidget * m_backtraceWidget;
        CrashInfo*           m_crashInfo;
};

//Bug Awareness Page ---------------
class BugAwarenessPage: public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        BugAwarenessPage(CrashInfo*);
        
        void aboutToHide();
       
    private:
        QCheckBox * m_canDetailCheckBox;
        QCheckBox * m_compromiseCheckBox;
        QCheckBox * m_canReproduceCheckBox;
        
        CrashInfo * m_crashInfo;
};

//Conclusions/Result page
class ConclusionPage : public DrKonqiAssistantPage
{
    Q_OBJECT
    
    public:
        ConclusionPage( CrashInfo* );
        
        void aboutToShow();
        
        bool isComplete();
        
    private Q_SLOTS:
        void reportButtonClicked();
        void saveReport();
        
    private:
        QLabel *        m_explanationLabel;
        KTextBrowser *  m_reportEdit;

        KPushButton *   m_reportButton;
        KPushButton *   m_saveReportButton;

        CrashInfo *     m_crashInfo;
        
        bool isBKO;
        bool needToReport;
};

#endif
