
class Private
{
public:
    void _k_slotHttpGetResult( KJob* job );
    void _k_slotData(KIO::Job*, const QByteArray&);

    QUrl ontoNamespace;
    KTemporaryFile tmpFile;
};


void start()
{
    KIO::TransferJob* job = KIO::get( d->ontoNamespace );
    connect( job, SIGNAL(result(KJob*)),
             this, SLOT(_k_slotHttpGetResult(KJob*)) );
    connect( job, SIGNAL(data(KIO::Job*, const QByteArray&)),
             this, SLOT(_k_slotData(KIO::Job*, const QByteArray&)) );
    job->start();
}


void Private::_k_data( KIO::Job*, const QByteArray& data )
{
    if( !d->tmpFile.isOpen() ) {
        d->tmpFile.open();
    }

    d->tmpFile.write( data );
}


void Private::_k_slotHttpGetResult( KJob* job )
{
    d->tmpFile.close();

    if( !job->error() ) {
        QString mimeType = job->mimetype();
        const Soprano::Parser* parser =
            Soprano::PluginManager::instance()->discoverParserForSerialization( Soprano::mimeTypeToSerialization( mimetype ),
                                                                                mimetype );
        if( parser ) {
            Soprano::StatementIterator it = parser->parseFile( d->tmpFile.fileName(),
                                                               d->ontoNamespace,
                                                               Soprano::mimeTypeToSerialization( mimetype ),
                                                               mimetype );

        }
        else {
            // FIXME: report error: unable to handle data of type XXX
        }
    }
    else {
        // FIXME: report http error
    }

    d->tmpFile.remove();
}
