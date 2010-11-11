#!/usr/bin/env python
# -*- coding: utf-8 -*-

from PyKDE4.nepomuk import Nepomuk
from PyKDE4.soprano import Soprano

import re

def trim( url ) :
    pattern = r"^http://www.*\d/(.*)$"
    m = re.match( pattern, url )
    if m :
        str = m.group(1)
        str.replace( '#', ':' )
        if str.contains("rdf-schema") :
            str.replace("rdf-schema", "rdfs")
        return str

query = "select distinct ?r where { ?r ?p ?o . ?r a rdf:Property. FILTER(!bif:exists( ( select distinct ?r where { ?r nrl:MaxCardinality ?o. } ) )) . }"

model = Nepomuk.ResourceManager.instance().mainModel();
it = model.executeQuery( query, Soprano.Query.QueryLanguageSparql )

list = []
while it.next() :
    uri = it[0].uri().toString()
    if uri.contains("semanticdesktop") or uri.contains("http://www.w3.org/") :
        list.append( trim( uri ) )

list.sort()
for l in list :
    print l
print len(list)