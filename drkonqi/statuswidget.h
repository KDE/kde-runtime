#ifndef STATUSWIDGET__H
#define STATUSWIDGET__H

#include <QtGui/QStackedWidget>

class QLabel;
class QProgressBar;

class StatusWidget: public QStackedWidget
{
    Q_OBJECT
    public:
        StatusWidget( QWidget * parent = 0);
        
        void setBusy( QString );
        void setIdle( QString );

        void addCustomStatusWidget( QWidget * );
        
        void setStatusLabelWordWrap( bool );
        
    private:
        QLabel *            m_statusLabel;
        
        QProgressBar *      m_progressBar;
        QLabel *            m_busyLabel;
        
        QWidget *           m_statusPage;
        QWidget *           m_busyPage;
};

#endif
