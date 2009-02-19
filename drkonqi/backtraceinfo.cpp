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

#include <QtCore>
#include "backtraceinfo.h"

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
        
        if( line.startsWith("#") ){
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

              //qDebug() << "linea" << line;

              if( isQtMeta(line) || isCrashHandler(line) || isUnuseful(line) )
                continue;

              //Line contains symbols ?
              if ( line.contains(".cpp:") || line.contains(".h:") || line.contains(".moc:"))
              {
                
                firstGoodFound = true;
                validSymbolsCount++;
                rating.insert(number, Good);
                //qDebug() << "agregando buena" << line;
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
                      if( !(!firstGoodFound && falsePositive(file) ) && usefulFile( file ) ) //  &&  || !nonImportant(file)
                      {
                          
                          if( line.contains("??") )
                          {
                              //qDebug() << "agregando malo" << file << line;
                              rating.insert(number, Missing);
                          } 
                          else 
                          {
                                //qDebug() << "agregando medio" << file << line;
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
                            number++;
                            rating.insert(number, MissingNumber);
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
      line.contains("KCrash::startDrKonqi") );
}

bool BacktraceInfo::isUnuseful( QString line )
{
    return line.contains( "__kernel_vsyscall"  );
}

QStringList BacktraceInfo::usefulFilesWithMissingSymbols()
{
    QStringList list;
    Q_FOREACH( const QString &key, missingSymbols.keys() )
    {
        if( missingSymbols.value( key ) < desiredFunctionCountSymbols )
                if( usefulFile( key ) ){
                    list.append( key );
                  }
    }
    return list;
}

bool BacktraceInfo::usefulFile( QString file )
{
    //KDE+qt
    QFileInfo fI(file);
    file = fI.fileName();
    
    QStringList usefulFiles;
    usefulFiles << "libkhtml.so.5";
    usefulFiles << "kdecore.so.4";
    usefulFiles << "libkdeui.so.5";
    usefulFiles << "libQtCore.so.4";
    usefulFiles << "libQtGui.so.4";
    usefulFiles << "libQtNetwork.so.4";
    usefulFiles << "kopete_wlm.so";
    usefulFiles << "katepart.so";
    usefulFiles << "libgwenviewlib.so.4";
    usefulFiles << "plasma_applet_folderview.so";
    usefulFiles << "libkio.so.5";
    usefulFiles << "libtaskmanager.so.4";
    usefulFiles << "kcm_kgamma.so";
    usefulFiles << "libkmailprivate.so.4";
    usefulFiles << "libsoprano.so.4";
    usefulFiles << "libplasma.so.3";
    usefulFiles << "libplasma.so.2";
    usefulFiles << "plasma_applet_systemtray.so";
    
    return usefulFiles.contains(file);
    
}

bool BacktraceInfo::falsePositive( QString file )
{
    //Non-kde files
    QFileInfo fI(file);
    file = fI.fileName();
    
    QStringList falseP;
    falseP << "libQtCore.so.4";
    falseP << "libQtGui.so.4";
    falseP << "libQtNetwork.so.4";
    falseP << "libc.so.6";
    falseP << "libstdc++.so.6";
    falseP << "libstdc++.so.5";
    falseP << "ld-linux.so.2";
    falseP << "libdl.so.2";
    
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
        //qDebug() << "rating" << rat;
        //qDebug() << "a" << linesCount * Missing;
        //qDebug() << "b" << linesCount * MissingNumber;

        //Calculate best rating
        int bestRating = 0;
        for(int i=2; i <= linesCount +1; i++)
            bestRating += Good*i;
            
        int maybeUsefulRating = ((int)bestRating/3.0)*2;
        int probablyUnusefulRating = (int)bestRating/3.0;
        
        if(rat == bestRating)
        {
            usefulness = ReallyUseful;
        }
        else if(rat > maybeUsefulRating && rat < bestRating)
        {
            usefulness = MayBeUseful;
        } 
        
        else if(rat > probablyUnusefulRating && rat <= maybeUsefulRating)
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

/*
int BacktraceInfo::getUsefulnessValue()
{
    int val = 0;
    switch(usefulness){
        case ReallyUseful:
            val = 100;break;
        case MayBeUseful:
            val = 60;break;
        case ProbablyUnuseful:
            val = 30;break;
        case Unuseful:
            val = 0;break;
    }
    return val;

}
*/

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