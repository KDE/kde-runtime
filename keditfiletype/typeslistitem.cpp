
#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>

#include "typeslistitem.h"

TypesListItem::TypesListItem(QListView *parent, const QString & major)
  : QListViewItem(parent), metaType(true), m_bNewItem(false)
{
  initMeta(major);
  setText(0, majorType());
}

TypesListItem::TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype, bool newItem)
  : QListViewItem(parent), metaType(false), m_bNewItem(newItem)
{
  init(mimetype);
  setText(0, minorType());
}

TypesListItem::TypesListItem(QListView *parent, KMimeType::Ptr mimetype)
  : QListViewItem(parent), metaType(false), m_bNewItem(false)
{
  init(mimetype);
  setText(0, majorType());
}

TypesListItem::~TypesListItem()
{
}

void TypesListItem::initMeta( const QString & major )
{
  m_bFullInit = true;
  m_mimetype = 0L;
  m_major = major;
  KConfig config("konquerorrc", true);
  config.setGroup("EmbedSettings");
  bool defaultValue = defaultEmbeddingSetting( major );
  m_autoEmbed = config.readBoolEntry( QString::fromLatin1("embed-")+m_major, defaultValue ) ? 0 : 1;
}

bool TypesListItem::defaultEmbeddingSetting( const QString& major )
{
  return (major=="image"); // embedding is false by default except for image/*
}

void TypesListItem::setup()
{
  if (m_mimetype)
  {
     setPixmap(0, m_mimetype->pixmap(KIcon::Small));
  }
  QListViewItem::setup();
}

void TypesListItem::init(KMimeType::Ptr mimetype)
{
  m_bFullInit = false;
  m_mimetype = mimetype;

  int index = mimetype->name().find("/");
  if (index != -1) {
    m_major = mimetype->name().left(index);
    m_minor = mimetype->name().right(mimetype->name().length() -
                                     (index+1));
  } else {
    m_major = mimetype->name();
    m_minor = "";
  }
  m_comment = mimetype->comment(QString(), false);
  m_icon = mimetype->icon(QString(), false);
  m_patterns = mimetype->patterns();

  QVariant v = mimetype->property( "X-KDE-AutoEmbed" );
  m_autoEmbed = v.isValid() ? (v.toBool() ? 0 : 1) : 2;
}

QStringList TypesListItem::appServices() const
{
  if (!m_bFullInit)
  {
     TypesListItem *that = const_cast<TypesListItem *>(this);
     that->getServiceOffers(that->m_appServices, that->m_embedServices);
     that->m_bFullInit = true;
  }
  return m_appServices;
}

QStringList TypesListItem::embedServices() const
{
  if (!m_bFullInit)
  {
     TypesListItem *that = const_cast<TypesListItem *>(this);
     that->getServiceOffers(that->m_appServices, that->m_embedServices);
     that->m_bFullInit = true;
  }
  return m_embedServices;
}

void TypesListItem::getServiceOffers( QStringList & appServices, QStringList & embedServices ) const
{
  KServiceTypeProfile::OfferList offerList =
    KServiceTypeProfile::offers(m_mimetype->name(), "Application");
  QValueListIterator<KServiceOffer> it(offerList.begin());
  for (; it != offerList.end(); ++it)
    if ((*it).allowAsDefault())
      appServices.append((*it).service()->desktopEntryPath());

  offerList = KServiceTypeProfile::offers(m_mimetype->name(), "KParts/ReadOnlyPart");
  for ( it = offerList.begin(); it != offerList.end(); ++it)
    embedServices.append((*it).service()->desktopEntryPath());
}

bool TypesListItem::isDirty() const
{
  if ( !m_bFullInit)
  {
    return false;
  }

  if ( m_bNewItem )
  {
    kdDebug() << "New item, need to save it" << endl;
    return true;
  }

  if ( !isMeta() )
  {

    if ((m_mimetype->name() != name()) &&
        (name() != "application/octet-stream"))
    {
      kdDebug() << "Mimetype Name Dirty: old=" << m_mimetype->name() << " name()=" << name() << endl;
      return true;
    }
    if (m_mimetype->comment(QString(), false) != m_comment)
    {
      kdDebug() << "Mimetype Comment Dirty: old=" << m_mimetype->comment(QString(),false) << " m_comment=" << m_comment << endl;
      return true;
    }
    if (m_mimetype->icon(QString(), false) != m_icon)
    {
      kdDebug() << "Mimetype Icon Dirty: old=" << m_mimetype->icon(QString(),false) << " m_icon=" << m_icon << endl;
      return true;
    }

    if (m_mimetype->patterns() != m_patterns)
    {
      kdDebug() << "Mimetype Patterns Dirty: old=" << m_mimetype->patterns().join(";")
                << " m_patterns=" << m_patterns.join(";") << endl;
      return true;
    }

    // not used anywhere?
//     KServiceTypeProfile::OfferList offerList =
//       KServiceTypeProfile::offers(m_mimetype->name());

    QStringList oldAppServices;
    QStringList oldEmbedServices;
    getServiceOffers( oldAppServices, oldEmbedServices );

    if (oldAppServices != m_appServices)
    {
      kdDebug() << "App Services Dirty: old=" << oldAppServices.join(";")
                << " m_appServices=" << m_appServices.join(";") << endl;
      return true;
    }
    if (oldEmbedServices != m_embedServices)
    {
      kdDebug() << "Embed Services Dirty: old=" << oldEmbedServices.join(";")
                << " m_embedServices=" << m_embedServices.join(";") << endl;
      return true;
    }

    QVariant v = m_mimetype->property( "X-KDE-AutoEmbed" );
    unsigned int oldAutoEmbed = v.isValid() ? (v.toBool() ? 0 : 1) : 2;
    if ( oldAutoEmbed != m_autoEmbed )
      return true;

  } else {

    KConfig config("konquerorrc", true);
    config.setGroup("EmbedSettings");
    bool defaultValue = defaultEmbeddingSetting(m_major);
    unsigned int oldAutoEmbed = config.readBoolEntry( QString::fromLatin1("embed-")+m_major, defaultValue ) ? 0 : 1;
    if ( m_autoEmbed != oldAutoEmbed )
      return true;
  }

  // nothing seems to have changed, it's not dirty.
  return false;
}

void TypesListItem::sync()
{
  Q_ASSERT(m_bFullInit);
  if ( isMeta() )
  {
    KConfig config("konquerorrc");
    config.setGroup("EmbedSettings");
    config.writeEntry( QString::fromLatin1("embed-")+m_major, m_autoEmbed == 0 );
    return;
  }
  QString loc = name() + ".desktop";
  loc = locateLocal("mime", loc);

  KSimpleConfig config( loc );
  config.setDesktopGroup();

  config.writeEntry("Type", "MimeType");
  config.writeEntry("MimeType", name());
  config.writeEntry("Icon", m_icon);
  config.writeEntry("Patterns", m_patterns, ';');
  config.writeEntry("Comment", m_comment);
  config.writeEntry("Hidden", false);

  if ( m_autoEmbed == 2 )
    config.deleteEntry( QString::fromLatin1("X-KDE-AutoEmbed"), false );
  else
    config.writeEntry( QString::fromLatin1("X-KDE-AutoEmbed"), m_autoEmbed == 0 );

  m_bNewItem = false;

  KConfig profile("profilerc", false, false);

  // Deleting current contents in profilerc relating to
  // this service type
  //
  QStringList groups = profile.groupList();

  for (QStringList::Iterator it = groups.begin();
       it != groups.end(); it++ )
  {
    profile.setGroup(*it);

    // Entries with Preference <= 0 or AllowAsDefault == false
    // are not in m_services
    if ( profile.readEntry( "ServiceType" ) == name()
         && profile.readNumEntry( "Preference" ) > 0
         && profile.readBoolEntry( "AllowAsDefault" ) )
    {
      profile.deleteGroup( *it );
    }
  }

  // Save preferred services
  //

  groupCount = 1;

  saveServices( profile, m_appServices, "Application" );
  saveServices( profile, m_embedServices, "KParts/ReadOnlyPart" );

  // Handle removed services

  KServiceTypeProfile::OfferList offerList =
    KServiceTypeProfile::offers(m_mimetype->name(), "Application");
  offerList += KServiceTypeProfile::offers(m_mimetype->name(), "KParts/ReadOnlyPart");

  QValueListIterator<KServiceOffer> it_srv(offerList.begin());

  for (; it_srv != offerList.end(); ++it_srv) {


      KService::Ptr pService = (*it_srv).service();

      bool isApplication = pService->type() == "Application";
      if (isApplication && !pService->allowAsDefault())
          continue; // Only those which were added in init()

      // Look in the correct list...
      if ( (isApplication && ! m_appServices.contains( pService->desktopEntryPath() ))
           || (!isApplication && !m_embedServices.contains( pService->desktopEntryPath() ))
          ) {
        // The service was in m_appServices but has been removed
        // create a new .desktop file without this mimetype

        QStringList serviceTypeList = pService->serviceTypes();

        if ( serviceTypeList.contains( name() ) ) {
          // The mimetype is listed explicitly in the .desktop files, so
          // just remove it and we're done
          KConfig *desktop;
          if ( !isApplication )
          {
            desktop = new KConfig(pService->desktopEntryPath(), false, false, "services");
          }
          else
          {
            QString path = KDesktopFile::locateLocal(pService->desktopEntryPath());
            KConfig orig(pService->desktopEntryPath(), true, false, "apps");
            desktop = orig.copyTo(path);
          }
          
          serviceTypeList.remove(name());
          desktop->writeEntry("MimeType", serviceTypeList, ';');

          desktop->sync();
          delete desktop;
        }
        else {
          // The mimetype is not listed explicitly so it can't
          // be removed. Preference = 0 handles this.

          // Find a group header. The headers are just dummy names as far as
          // KUserProfile is concerned, but using the mimetype makes it a
          // bit more structured for "manual" reading
          while ( profile.hasGroup(
                  name() + " - " + QString::number(groupCount) ) )
              groupCount++;

          profile.setGroup( name() + " - " + QString::number(groupCount) );

          profile.writeEntry("Application", pService->desktopEntryPath());
          profile.writeEntry("ServiceType", name());
          profile.writeEntry("AllowAsDefault", true);
          profile.writeEntry("Preference", 0);
        }
      }
  }
}

void TypesListItem::saveServices( KConfig & profile, QStringList services, const QString & genericServiceType )
{
  QStringList::Iterator it(services.begin());
  for (int i = services.count(); it != services.end(); ++it, i--) {

    KService::Ptr pService = KService::serviceByDesktopPath(*it);
    Q_ASSERT(pService);

    // Find a group header. The headers are just dummy names as far as
    // KUserProfile is concerned, but using the mimetype makes it a
    // bit more structured for "manual" reading
    while ( profile.hasGroup( name() + " - " + QString::number(groupCount) ) )
        groupCount++;

    profile.setGroup( name() + " - " + QString::number(groupCount) );

    profile.writeEntry("ServiceType", name());
    profile.writeEntry("GenericServiceType", genericServiceType);
    profile.writeEntry("Application", pService->desktopEntryPath());
    profile.writeEntry("AllowAsDefault", true);
    profile.writeEntry("Preference", i);

    KConfig *desktop;
    if ( pService->type() == QString("Service") )
    {
        desktop = new KConfig(pService->desktopEntryPath(), false, false, "services");
    }
    else
    {
        QString path = KDesktopFile::locateLocal(pService->desktopEntryPath());
        KConfig orig(pService->desktopEntryPath(), true, false, "apps");
        desktop = orig.copyTo(path);
    }
    // merge new mimetype
    QStringList serviceTypeList = pService->serviceTypes();

    if (!serviceTypeList.contains(name()))
      serviceTypeList.append(name());

    desktop->writeEntry("MimeType", serviceTypeList, ';');
    desktop->writeEntry("ServiceTypes", "");
    desktop->sync();
    delete desktop;
  }
}

void TypesListItem::setIcon( const QString& icon )
{
  m_icon = icon;
  setPixmap( 0, SmallIcon( icon ) );
}

bool TypesListItem::isEssential() const
{
    QString n = name();
    if ( n == "application/octet-stream" )
        return true;
    if ( n == "inode/directory" )
        return true;
    if ( n == "inode/directory-locked" )
        return true;
    if ( n == "inode/blockdevice" )
        return true;
    if ( n == "inode/chardevice" )
        return true;
    if ( n == "inode/socket" )
        return true;
    if ( n == "inode/fifo" )
        return true;
    if ( n == "application/x-shellscript" )
        return true;
    if ( n == "application/x-executable" )
        return true;
    if ( n == "application/x-desktop" )
        return true;
    return false;
}

void TypesListItem::refresh()
{
    kdDebug() << "TypesListItem refresh " << name() << endl;
    m_mimetype = KMimeType::mimeType( name() );
}
