#ifndef USEFULNESSLABEL_H
#define USEFULNESSLABEL_H

#include <QtGui/QPixmap>
#include <QtGui/QWidget>

#include "backtraceinfo.h"
#include "crashinfo.h"

class UsefulnessMeter: public QWidget
{
    Q_OBJECT
    
    public:
        
        UsefulnessMeter(QWidget *);
        void setUsefulness( BacktraceInfo::Usefulness );
        void setState( CrashInfo::BacktraceGenState s ) { state = s; update(); }
        
    protected:
        
        void paintEvent ( QPaintEvent * event );
        
    private:
    
        CrashInfo::BacktraceGenState state;
        
        bool star1;
        bool star2;
        bool star3;
        
        QPixmap errorPixmap;
        QPixmap loadedPixmap;
        QPixmap loadingPixmap;
        
        QPixmap starPixmap;
        QPixmap disabledStarPixmap;
};

#endif
