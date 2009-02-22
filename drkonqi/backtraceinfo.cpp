/*******************************************************************
* backtraceinfo.cpp
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

#include "backtraceinfo.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>

//#define DEBUG

#include <QtDebug>

static const int desiredFunctionCountSymbols = 4;

BacktraceInfo::BacktraceInfo()
{
     validSymbolsCount = 0;
     invalidSymbolsCount = 0;
     maybeUsefulSymbolsCount = 0;
     usefulness = Unuseful;
}

void BacktraceInfo::setBacktraceData( QByteArray data )
{
    backtraceData = data;
    fixLines();
}

//Guess start position of the KCrash handler
int BacktraceInfo::guessStartPosition()
{
    int pos = backtraceData.indexOf("KCrash");
    if ( pos == -1 )
        pos = backtraceData.indexOf("Konqui", pos);
    if ( pos == -1 )
        pos = backtraceData.indexOf("crash", pos);
    if ( pos == -1 )
        pos = backtraceData.indexOf("<signal handler called>", pos);

    return pos;
}

//Fix the backtraces lines
void BacktraceInfo::fixLines() 
{
    QByteArray data;
    QTextStream ts(&backtraceData, QIODevice::ReadOnly);
    QByteArray line;
    while( !ts.atEnd() )
    {
        line = ts.readLine().toLocal8Bit();
        
        if( line.startsWith('#') ){
            data.append('\n' + line);
        } else {
            data.append("  " + line);
        }
    }
    backtraceData = data.mid(1,data.size());
}

void BacktraceInfo::parse()
{
  if( ! backtraceData.isEmpty() )
  {
      int position = guessStartPosition();
      bool firstGoodFound = false;
      
      if( position != -1) //Valid backtrace
      {
          QTextStream ts(&backtraceData, QIODevice::ReadOnly);
          ts.seek( position );
          
          QString line;
          line = "foo";
          
          int number = 0;
          while( line!= QLatin1String("") ){
              
              line = ts.readLine();

              //qDebug() << "line" << line;

              if( isQtMeta(line) || isCrashHandler(line) || isUnusefulLine(line) )
                continue;

              //Line contains symbols ?
              if ( line.contains(".cpp:") || line.contains(".h:") || line.contains(".moc:"))
              {
                
                firstGoodFound = true;
                validSymbolsCount++;
                addValidFunction( line );
                rating.insert(number, Good);
                
                #ifdef DEBUG
                    qDebug() << "Good line" << line;
                #endif
                number++;
                //importance--;
              }
              else
              {
                  //On miss symbols, fetch the filename
                  QString file = line.split(' ').last();
                  //qDebug() << file;
                  QFileInfo fI( file );
                  
                  if( fI.isAbsolute() ) // Valid file name?
                  {
                      //qDebug() << "la";
                      //Avoid qt basic functions that may be before the first important (valid) line
                      if( !(!firstGoodFound && isQtFile(file) ) && isUsefulFile( file ) ) //  &&  || !nonImportant(file)
                      {
                          
                          if( line.contains("??") )
                          {
                                #ifdef DEBUG
                                    qDebug() << "Bad line" << file << line;
                                #endif
                              rating.insert(number, Missing);
                          } 
                          else 
                          {
                                #ifdef DEBUG
                                qDebug() << "Medium line" << file << line;
                                #endif
                                addValidFunction( line );
                                rating.insert(number, MissingNumber);
                          }
                          number++;
                          
                          //qDebug() << "por agregar" << file;
                          if( !missingSymbols.contains( file ) )
                          {
                              //qDebug() << "agregando" << file << number;
                              missingSymbols.insert( file, number );
                              maybeUsefulSymbolsCount++;
                          }
                    
                      }
                      
                  } else {
                        
                        //Missing filename ?
                        if ( line.contains("()") )
                        {
                            if ( !line.contains("??") )
                            {
                                #ifdef DEBUG
                                qDebug() << "Medium line" << line;
                                #endif
                                rating.insert(number, MissingNumber);
                                number++;
                            }
                            else
                            {
                                #ifdef DEBUG
                                qDebug() << "Bad line" << line;
                                #endif
                                rating.insert(number, Missing);
                                number++;
                            }
                        }   
                 
                  }
              }//if (symbols)
              
              //
              

          }//while
      
          calculateUsefulness();
          
      } // if( position != -1) //Valid backtrace
  
  } // if( ! backtraceData.isEmpty() )

}

bool BacktraceInfo::isQtMeta( QString line )
{
  return ( line.contains("in QMetaObject") ||  //line.contains("qt_metacall") ||
    line.contains("in QApplicationPrivate::notify_helper") || 
    line.contains("in QApplication::notify") || 
    line.contains("in KApplication::notify") || 
    line.contains("in QCoreApplication::notifyInternal") ||
    line.contains("in qt_message_output") );
}
bool BacktraceInfo::isCrashHandler( QString line )
{
    return ( line.contains("(no debugging symbols found)") || 
      line.contains("KCrash::defaultCrashHandler") || 
      line.contains("<signal handler called>") || 
      line.contains("KCrash::startDrKonqi") ||
      line.contains("KCrash handler")
    );
}

bool BacktraceInfo::isUnusefulLine( QString line )
{
    return line.contains( "__kernel_vsyscall"  );
}

QStringList BacktraceInfo::usefulFilesWithMissingSymbols()
{
    QStringList list;
    Q_FOREACH( const QString &key, missingSymbols.keys() )
    {
        if( missingSymbols.value( key ) < desiredFunctionCountSymbols )
                if( isUsefulFile( key ) ){
                    list.append( key );
                  }
    }
    return list;
}

bool BacktraceInfo::isUsefulFile( QString file )
{
    QFileInfo fI(file);
    file = fI.fileName();

    //unsefulFileList
    QStringList unusefulFiles;
    unusefulFiles << "libc";
    unusefulFiles << "libstdc++";
    unusefulFiles << "ld-linux";
    unusefulFiles << "libdl";
    unusefulFiles << "libGL";
    
    bool useful = true;
    Q_FOREACH( const QString & filePattern , unusefulFiles)
    {
        if (file.contains(filePattern))
            useful = false;
    }
    
    return useful;
}

bool BacktraceInfo::isQtFile( QString file )
{
    //Qt files
    QFileInfo fI(file);
    file = fI.fileName();
    
    QStringList falseP;
    falseP << "libQtCore.so.4";
    falseP << "libQtGui.so.4";
    falseP << "libQtNetwork.so.4";
        
    return falseP.contains(file);
  
}

void BacktraceInfo::calculateUsefulness()
{
    int rat = 0;
    QList<int> keys = rating.keys();
    int linesCount = keys.size() >= desiredFunctionCountSymbols ? desiredFunctionCountSymbols : keys.size();

    if ( linesCount != 0 )
    {
        //qDebug() << "count" << linesCount;
        for(int i = 0; i < linesCount  ; ++i)
        {
            rat += rating.value( keys.at( i ) ) * (linesCount-i+1);
        }
        
        #ifdef DEBUG
        qDebug() << "rating" << rat;
        #endif
        //qDebug() << "a" << linesCount * Missing;
        //qDebug() << "b" << linesCount * MissingNumber;

        //Calculate best (possible) rating
        int bestRating = 0;
        for(int i=2; i <= linesCount +1; i++)
            bestRating += Good*i;
        
        //qDebug() << "best" << bestRating;
        
        int maybeUsefulRating = ((int)bestRating/4.0)*3;
        int probablyUnusefulRating = (int)(bestRating/4.0*1.8);
        
        //qDebug() << "prob-un" << probablyUnusefulRating;
        
        if(rat == bestRating)
        {
            usefulness = ReallyUseful;
        }
        else if(rat > maybeUsefulRating && rat < bestRating)
        {
            usefulness = MayBeUseful;
        } 
        
        else if(rat >= probablyUnusefulRating && rat <= maybeUsefulRating)
        {
            usefulness = ProbablyUnuseful;
        }
        else
        {
            usefulness = Unuseful;
        }
        
    }
    else
    {
        usefulness = Unuseful;
    }
      
}

void BacktraceInfo::addValidFunction( QString f )
{
    // , " ("
    QString functionName = f.mid(f.indexOf(" in ")+4);
    functionName = functionName.left( functionName.indexOf("(") );
    functionName = functionName.trimmed();
    
    if( firstValidFunctions.split(' ').count() < 3 )
        firstValidFunctions.append( QLatin1String(" ") + functionName );
}

QString BacktraceInfo::getUsefulnessString()
{
    QString text;
    switch(usefulness){
        case ReallyUseful:
            text = "The crash information is really useful";break;
        case MayBeUseful:
            text = "The crash information may be useful";break;
        case ProbablyUnuseful:
            text = "The crash information is probably unuseful";break;
        case Unuseful:
            text = "The crash information is definetly unuseful";break;
    }
    return text;

}
