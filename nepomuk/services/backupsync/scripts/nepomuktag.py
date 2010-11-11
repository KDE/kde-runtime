#!/usr/bin/python

from PyKDE4.nepomuk import Nepomuk
from PyKDE4.soprano import Soprano
from PyQt4 import QtCore

import sys

#
# Check number of arguments and print help message
#
if len(sys.argv) < 2 :
    print("Correct usage:", sys.argv[0], " list/add/remove/tag/show")
    quit()

resourceManager = Nepomuk.ResourceManager.instance()
print("")

argv = sys.argv[1:]
command = argv[0]

if( command == "list" ) :
    print("Tags -- ")
    tags =  Nepomuk.Tag.allTags()
    for tag in tags :
        print(tag.genericLabel())

elif command == "add" :
    tagName = argv[1]
    tag = Nepomuk.Tag( tagName )
    if tag.exists() :
        print(tagName, " already exists!")
    else :
        tag.setProperty( "http://www.semanticdesktop.org/ontologies/2007/08/15/nao#prefLabel", Nepomuk.Variant(tagName) )

elif command == "remove" :
    tagName = argv[1]
    tag = Nepomuk.Tag( tagName )
    if not tag.exists() :
        print(tagName, " doesn't exist!")
    else :
        tag.remove()

elif command == "tag" :
    file = argv[1]
    tagName = argv[2]
    tag = Nepomuk.Tag( tagName )
    if not tag.exists() :
        print(tagName, " doesn't exist!")
        quit()
    else :
        res = Nepomuk.Resource( file )
        res.addTag( tag )
        print("Added tag")

elif command == "show" :
    tagName = argv[1]
    tag = Nepomuk.Tag( tagName )
    if tag.exists() :
        tagOf = tag.tagOf()
        for res in tagOf :
            print(res.property("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url").toString())
    else :
        print(tagName, " doesn't exist")