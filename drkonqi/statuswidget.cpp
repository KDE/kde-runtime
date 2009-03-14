#include "statuswidget.h"

#include <QtGui/QSizePolicy>

#include <QtGui/QProgressBar>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

#include <klocale.h>

StatusWidget::StatusWidget( QWidget * parent ) :
    QStackedWidget( parent )
{
    //Main layout
    m_statusPage = new QWidget( this );
    m_busyPage = new QWidget( this );
    
    insertWidget( 0, m_statusPage );
    insertWidget( 1, m_busyPage );
    
    //Status widget
    m_statusLabel = new QLabel();
    m_statusLabel->setOpenExternalLinks( true );
    m_statusLabel->setTextFormat( Qt::RichText );
    
    QHBoxLayout * statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins( 0,0,0,0 );
    statusLayout->setSpacing( 2 );
    m_statusPage->setLayout( statusLayout );
    
    statusLayout->addWidget( m_statusLabel );
    
    //Busy widget
    m_progressBar = new QProgressBar();
    m_progressBar->setRange( 0, 0 );
    
    m_busyLabel = new QLabel( i18n( "Busy ...") );
    //m_label->setWordWrap( true );
    
    QHBoxLayout * busyLayout = new QHBoxLayout();
    busyLayout->setContentsMargins( 0,0,0,0 );
    busyLayout->setSpacing( 2 );
    m_busyPage->setLayout( busyLayout );
    
    busyLayout->addWidget( m_busyLabel );
    busyLayout->addStretch();
    busyLayout->addWidget( m_progressBar );
    
    setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum ) );
}

void StatusWidget::setBusy( QString busyMessage )
{
    m_statusLabel->setText( QString() );
    m_busyLabel->setText( busyMessage );
    setCurrentIndex( 1 );
}

void StatusWidget::setIdle( QString idleMessage )
{
    m_busyLabel->setText( QString() );
    m_statusLabel->setText( idleMessage );
    setCurrentIndex( 0 );
}

void StatusWidget::addCustomStatusWidget( QWidget * widget )
{
    QVBoxLayout * statusLayout = static_cast<QVBoxLayout*>(m_statusPage->layout());
    
    statusLayout->addStretch();
    statusLayout->addWidget( widget );
}

void StatusWidget::setStatusLabelWordWrap( bool ww )
{
    m_statusLabel->setWordWrap( ww );
}