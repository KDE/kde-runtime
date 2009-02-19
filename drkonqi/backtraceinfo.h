/*******************************************************************
* backtraceinfo.h
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

#ifndef BACKTRACEINFO__H
#define BACKTRACEINFO__H

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