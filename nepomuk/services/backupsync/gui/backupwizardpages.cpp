/*
    This file is part of the Nepomuk KDE project.
    Copyright (C) 2010  Vishesh Handa <handa.vish@gmail.com>
    Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "backupwizardpages.h"
#include "backupwizard.h"

#include "identifierwidget.h"

#include <KDebug>
#include <KLineEdit>
#include <KStandardDirs>
#include <KFileDialog>
#include <KUrlRequester>

#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>

Nepomuk::IntroPage::IntroPage(QWidget* parent)
    : QWizardPage( parent )
{
    setupUi( this );
    setTitle( i18n("Nepomuk Backup") );
    setSubTitle( i18n("Please choose one of the following options") );
}

int Nepomuk::IntroPage::nextId() const
{
    if( m_backup->isChecked() )
        return BackupWizard::Id_BackupSettingsPage;
    else if( m_restore->isChecked() )
        return BackupWizard::Id_RestoreSelectionPage;

    return -1;
}


//
// Backup Page
//
Nepomuk::BackupPage::BackupPage(QWidget* parent)
    : QWizardPage(parent),
      m_backupDone(false)
{
    setupUi( this );
    setTitle( i18n("Nepomuk Backup") );
    setSubTitle( i18n("Performing backup") );
    setCommitPage(true);

    m_backupManager = new BackupManager( QLatin1String("org.kde.nepomuk.services.nepomukbackupsync"),
                                         QLatin1String("/backupmanager"),
                                         QDBusConnection::sessionBus(), this);
    connect( m_backupManager, SIGNAL(backupDone()), this, SLOT(slotBackupDone()) );
}

void Nepomuk::BackupPage::initializePage()
{
    m_backupDone = false;
    KUrl backupUrl = field(QLatin1String("backupUrl")).value<KUrl>();
    kDebug() << backupUrl;
    m_status->setText( i18nc("@info", "Writing Nepomuk database backup to <filename>%1</filename>...",
                             field(QLatin1String("backupUrl")).value<KUrl>().pathOrUrl()));
    m_backupManager->backup( backupUrl.toLocalFile() );
}

bool Nepomuk::BackupPage::isComplete() const
{
    return m_backupDone;
}

int Nepomuk::BackupPage::nextId() const
{
    return -1;
}

void Nepomuk::BackupPage::slotBackupDone()
{
    m_backupDone = true;
    m_status->setText( i18nc("@info","Backup of the Nepomuk database successfully written to <filename>%1</filename>.",
                             field(QLatin1String("backupUrl")).value<KUrl>().pathOrUrl()) );
    setSubTitle( i18n("Backup completed successfully") );
    m_progressBar->setMaximum( 100 );
    m_progressBar->setValue( 100 );

    emit completeChanged();
}

//
// Restore Selection Page
//

Nepomuk::RestoreSelectionPage::RestoreSelectionPage(QWidget* parent): QWizardPage(parent)
{
    setupUi( this );
}

void Nepomuk::RestoreSelectionPage::initializePage()
{
    QDir dir( KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backups/" ) );
    QStringList backupFiles = dir.entryList( QDir::Files | QDir::NoDotAndDotDot, QDir::Name );

    foreach( const QString & backup, backupFiles ) {
        m_listWidget->addItem( backup );
    }

    if( backupFiles.isEmpty() ) {
        QLabel * errorLabel = new QLabel( i18nc("@info", "No system backups found. Please select a custom backup path.") , this);
        QGridLayout* layout = new QGridLayout(m_listWidget);
        layout->addWidget(errorLabel, 1, 1);
        layout->setRowStretch(0,1);
        layout->setRowStretch(2,1);
        layout->setColumnStretch(0,1);
        layout->setColumnStretch(2,1);
    }

    connect( m_customBackupButton, SIGNAL(clicked(bool)), this, SLOT(slotCustomBackupUrl()) );
    connect( m_listWidget, SIGNAL(itemSelectionChanged()),
             this, SLOT(slotSelectionChanged()) );

    registerField( "backupToRestorePath", this, "backupFilePath" );
}

bool Nepomuk::RestoreSelectionPage::isComplete() const
{
    return QFile::exists(m_backupFilePath);
}

int Nepomuk::RestoreSelectionPage::nextId() const
{
    return BackupWizard::Id_RestorePage;
}


void Nepomuk::RestoreSelectionPage::slotSelectionChanged()
{
    if( QListWidgetItem* item = m_listWidget->currentItem() )
        m_backupFilePath = KStandardDirs::locateLocal("data", QLatin1String("nepomuk/backupsync/backups/") + item->data( Qt::DisplayRole ).toString() );
    else
        m_backupFilePath.truncate(0);
    kDebug() << m_backupFilePath;
    emit completeChanged();
}

void Nepomuk::RestoreSelectionPage::slotCustomBackupUrl()
{
    m_backupFilePath = KFileDialog::getOpenFileName( KUrl(), QString(), this );
    kDebug() << "NEW BACKUP URL : " << m_backupFilePath;
    if( isComplete() ) {
        wizard()->next();
    }
}


//
// Restore Page
//
Nepomuk::RestorePage::RestorePage(QWidget* parent)
    : QWizardPage(parent)
{
    // Page Properties
    setTitle( i18n("Restoring Backup") );
    setSubTitle( i18n("The backup is being restored...") );

    m_backupManager = new BackupManager( QLatin1String("org.kde.nepomuk.services.nepomukbackupsync"),
                                         "/backupmanager",
                                         QDBusConnection::sessionBus(), this);
    m_identifier = Identifier::instance();

    connect( m_identifier, SIGNAL(identificationDone(int,int)),
             this, SLOT(slotIdentificationDone(int,int)) );
}


void Nepomuk::RestorePage::initializePage()
{
    QString backupUrl = field("backupToRestorePath").toString();
    kDebug() << "Restoring : " << backupUrl;


    if( backupUrl.isEmpty() )
        backupUrl = KStandardDirs::locateLocal( "data", "nepomuk/backupsync/backup" );

    m_id = Identifier::instance()->process( SyncFile(backupUrl) );

    if( m_id == -1 ) {
        //FIXME: This isn't implemented in the service. It's just there so that we have a
        // string that can be translated.
        kDebug() << "Invalid sync file";

        QLabel * invalidLabel = new QLabel( i18n("Invalid backup file"), this );
        m_identifierWidget->hide();
        layout()->addWidget( invalidLabel );
    }


    QHBoxLayout * layout = new QHBoxLayout( this );
    setLayout( layout );

    m_identifierWidget = new IdentifierWidget( m_id, this );
    layout->addWidget( m_identifierWidget );
}

int Nepomuk::RestorePage::nextId() const
{
    return BackupWizard::Id_RestoreFinalPage;
}

bool Nepomuk::RestorePage::validatePage()
{
    m_identifier->completeIdentification( m_id );
    return true;
}

void Nepomuk::RestorePage::slotIdentificationDone(int id, int unidentified)
{
    if( id == m_id && unidentified == 0 ) {
        wizard()->next();
    }
}

//
// Backup Settings Page
//

Nepomuk::BackupSettingsPage::BackupSettingsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setupUi(this);
    setTitle( i18n("Nepomuk Backup") );
    setSubTitle( i18n("Please configure the Nepomuk backup") );
    connect(m_editBackupUrl, SIGNAL(textChanged(QString)),
            this, SIGNAL(completeChanged()));
    connect(m_editBackupUrl, SIGNAL(urlSelected(KUrl)),
            this, SIGNAL(completeChanged()));

    registerField(QLatin1String("backupUrl"), this, "backupUrl");
}

KUrl Nepomuk::BackupSettingsPage::backupUrl() const
{
    return m_editBackupUrl->url();
}

bool Nepomuk::BackupSettingsPage::isComplete() const
{
    const KUrl url = m_editBackupUrl->url();
    return  QDir( url.directory() ).exists() && url.isValid();
}

int Nepomuk::BackupSettingsPage::nextId() const
{
    return BackupWizard::Id_BackupPage;
}


//
// Backup Final Page
//

Nepomuk::RestoreFinalPage::RestoreFinalPage(QWidget* parent): QWizardPage(parent)
{
    setupUi( this );
    setCommitPage( true );
    m_merger = Merger::instance();
    connect( m_merger, SIGNAL(completed(int)), this, SLOT(slotDone(int)) );

    m_progressBar->setMinimum( 0 );
    m_progressBar->setMaximum( 100 );

    m_status->setText( i18nc("@info", "Merging the backup into the local Nepomuk database...") );
}

void Nepomuk::RestoreFinalPage::initializePage()
{
    QWizardPage::initializePage();
}

int Nepomuk::RestoreFinalPage::nextId() const
{
    return -1;
}

void Nepomuk::RestoreFinalPage::slotDone(int per)
{
    m_progressBar->setValue( per );
    if( per == 100 ) {
        m_status->setText( i18nc("@info", "Backup restored successfully") );
    }
}


Nepomuk::ErrorPage::ErrorPage( QWidget* parent )
    : QWizardPage(parent)
{
    setupUi(this);
    setFinalPage(true);
    m_labelPixmap->setPixmap(KIcon(QLatin1String("dialog-error")).pixmap(48,48));
    registerField( QLatin1String("errorMessage"), this, "errorMessage" );
}

QString Nepomuk::ErrorPage::message() const
{
    return m_labelMessage->text();
}

void Nepomuk::ErrorPage::setMessage(const QString& s)
{
    m_labelMessage->setText(s);
}

#include "backupwizardpages.moc"
