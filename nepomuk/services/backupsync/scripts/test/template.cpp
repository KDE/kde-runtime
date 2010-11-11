#include <QtCore>

#include <Soprano/Statement>
#include <Soprano/Node>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/NodeIterator>
#include <Soprano/PluginManager>

#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>

#include <Nepomuk/Types/Property>
#include <Nepomuk/Types/Class>

#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/ResourceTerm>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/QueryParser>

#include <KDebug>
#include <QDebug>

// Vocabularies
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/RDFS>
#include <Soprano/Vocabulary/NRL>
#include <Soprano/Vocabulary/NAO>
#include "nie.h"
#include "nfo.h"
#include "nmm.h"
#include "nco.h"

#include <KUrl>

using namespace Nepomuk;


int main() {
    if( ResourceManager::instance()->init() ) {
        qDebug() << "Nepomuk isn't running";
        return 0;
    }
    Soprano::QueryResultIterator it;
    Soprano::Model * model = ResourceManager::instance()->mainModel();
    
    // --- CODE
    
