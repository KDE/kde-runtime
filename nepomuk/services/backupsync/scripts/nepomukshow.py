#!/usr/bin/env python
# -*- coding: utf-8 -*-

from PyKDE4.nepomuk import Nepomuk
from PyKDE4.soprano import Soprano

import re
import sys

def readable( url ) :
    pattern = r"^http://www.*\d/(.*)$"
    m = re.match( pattern, url )
    if m :
        str = m.group(1)
        str.replace( '#', ':' )
        str.replace("rdf-schema", "rdfs")
        str.replace("22-rdf-syntax-ns", "rdf")
        return str
    return url
    
def parseArg( arg ) :
    pattern = "[^\]*:[^\]*"

    #m = re.match( pattern, arg )
    #if m :
    #    return arg
    if arg.startswith("nepomuk:/") :
        return "<" + arg + ">"
    if arg.startswith("/home/") :
        return "<file://" + arg + ">"
    if arg.startswith("file://") :
        return "<" + arg + ">"

    return arg

def printIterator( res, it ) :
    if not it.isValid() :
        print("Does not exist ..")
        return
       
    if it.next() :
        r = it["r"].toString()
        if r :
            print(r)
        else :
            print(res)
        
        printPropertiesAndObject( it )
        
    while it.next() :
        printPropertiesAndObject( it )
        

def printPropertiesAndObject( it ) : 
        pred = it["p"].toString()
        obj = it["o"].toString()

        print(("\t" + readable(pred) + "\t\t" + readable(obj)))

def printArg( arg ) :
    res = parseArg( arg )
    query = ""
    
    if "file://" in res :
        query = "select distinct ?r ?p ?o where { ?r ?p ?o. ?r nie:url " + res + " . }"         
    else :
        query = "select distinct ?r ?p ?o where { " + res + " ?p ?o. }"
    
    model = Nepomuk.ResourceManager.instance().mainModel();
    print( "Executing - " + query )
    it = model.executeQuery( query, Soprano.Query.QueryLanguageSparql )
    
    print("\n")
    printIterator( res, it )
    
    
if len(sys.argv) < 2 :
    print(("Correct usage:", sys.argv[0], " uri/filename/keyword"))
    quit()
    
argv = sys.argv[1:]
for arg in argv :
     
    if ':' not in arg and '/' not in arg :
        query = "select distinct ?r where { ?r ?p ?o. FILTER( regex(str(?r), '"+ arg + "' , 'i') ) . }" 
        model = Nepomuk.ResourceManager.instance().mainModel();
        it = model.executeQuery( query, Soprano.Query.QueryLanguageSparql )
        
        while it.next() :
            r = it["r"].toString();
            printArg( "<" + str(r) + ">"  )
    else : 
        printArg( arg ) 