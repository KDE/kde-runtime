#ifndef BACKTRACEINFO_H
#define BACKTRACEINFO_H

#include <QtCore>

class BacktraceInfo : public QObject
{
  Q_OBJECT

  
  public:

    enum Usefulness { ReallyUseful = 0, MayBeUseful=1, ProbablyUnuseful=2, Unuseful = 3 };
    enum LineRating { Good = 2, MissingNumber = 1, Missing = 0 };
    
    BacktraceInfo();
    
    void setBacktraceData( QByteArray );
    void parse();
    
    Usefulness getUsefulness() { return usefulness; };
    QString getUsefulnessString();
    
    QStringList usefulFilesWithMissingSymbols();    
    
    //int getUsefulnessValue();
    
    
  private:

    void calculateUsefulness();
    bool usefulFile( QString );
    bool falsePositive ( QString );
    
    bool isQtMeta( QString );
    bool isCrashHandler ( QString );
    bool isUnuseful ( QString );
    //bool nonImportantFile( QString );
    
    int guessStartPosition();
    //QByteArray lineFixed( QByteArray );
    void fixLines();
    
    QByteArray backtraceData;
    
    QHash<QString, int> missingSymbols;
    
    uint validSymbolsCount;
    uint maybeUsefulSymbolsCount;
    uint invalidSymbolsCount;
    
    Usefulness usefulness;
    
    
    QMap<int, LineRating> rating;
};

#endif