#!/usr/bin/env python
# -*- coding: utf-8 -*-

## This file is part of the Nepomuk KDE project.
## Copyright (C) 2011 Sebastian Trueg <trueg@kde.org>
## Copyright (C) 2011 Serebriyskiy Artem <v.for.vandal@gmail.com>
##
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation; either
## version 2.1 of the License, or (at your option) version 3, or any
## later version accepted by the membership of KDE e.V. (or its
## successor approved by the membership of KDE e.V.), which shall
## act as a proxy defined in Section 6 of version 3 of the license.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with this library.  If not, see <http://www.gnu.org/licenses/>.


import argparse
import sys
import os, errno
from PyKDE4.soprano import Soprano
from PyQt4 import QtCore

output_path = os.getcwd()
verbose = True

def normalizeName(name):
    "Normalize a class or property name to be used as a C++ entity."
    name.replace('-', '_')
    name.replace('.', '_')
    return name

def extractNameFromUri(uri):
    "Extract the class or property name from an entity URI. This is the last section of the URI"
    name = uri.toString().mid(uri.toString().lastIndexOf(QtCore.QRegExp('[#/:]'))+1)
    return normalizeName(name)

def makeFancy(name, cardinality):
    if name.startsWith("has"):
        name = name[3].toLower() + name.mid(4)
    if cardinality != 1:
        if name.endsWith('s'):
            return name + 'es'
        else:
            return name + 's'
    else:
        return name

def extractOntologyName(uri):
    "The name of the ontology is typically the section before the name of the entity"
    return uri.toString().section(QtCore.QRegExp('[#/:]'), -2, -2)

def mkdir_p(path):
    "Create a folder and all its missing parent folders"
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST:
            pass
        else: raise

def typeString(rdfType, cardinality):
    """
    Construct the C++/Qt type to be used for the given type and cardinality.
    Uses QUrl for all non-literal types
    """
    if (rdfType == Soprano.Vocabulary.XMLSchema.string() or rdfType == Soprano.Vocabulary.RDFS.Literal()) and cardinality != 1:
        return 'QStringList'

    simpleType = ''
    if rdfType == Soprano.Vocabulary.XMLSchema.integer(): simpleType = "qint64"
    elif rdfType == Soprano.Vocabulary.XMLSchema.negativeInteger(): simpleType = "qint64"
    elif rdfType == Soprano.Vocabulary.XMLSchema.nonNegativeInteger(): simpleType = "quint64"
    elif rdfType == Soprano.Vocabulary.XMLSchema.xsdLong(): simpleType = "qint64"
    elif rdfType == Soprano.Vocabulary.XMLSchema.unsignedLong(): simpleType = "quint64"
    elif rdfType == Soprano.Vocabulary.XMLSchema.xsdInt(): simpleType = "qint32"
    elif rdfType == Soprano.Vocabulary.XMLSchema.unsignedInt(): simpleType = "quint32"
    elif rdfType == Soprano.Vocabulary.XMLSchema.xsdShort(): simpleType = "qint16"
    elif rdfType == Soprano.Vocabulary.XMLSchema.unsignedShort(): simpleType = "quint16"
    elif rdfType == Soprano.Vocabulary.XMLSchema.xsdFloat(): simpleType = "double"
    elif rdfType == Soprano.Vocabulary.XMLSchema.xsdDouble(): simpleType = "double"
    elif rdfType == Soprano.Vocabulary.XMLSchema.boolean(): simpleType = "bool"
    elif rdfType == Soprano.Vocabulary.XMLSchema.date(): simpleType = "QDate"
    elif rdfType == Soprano.Vocabulary.XMLSchema.time(): simpleType = "QTime"
    elif rdfType == Soprano.Vocabulary.XMLSchema.dateTime(): simpleType = "QDateTime"
    elif rdfType == Soprano.Vocabulary.XMLSchema.string(): simpleType = "QString"
    elif rdfType == Soprano.Vocabulary.RDFS.Literal(): simpleType = "QString"
    else: simpleType = 'QUrl'

    if cardinality != 1:
        return 'QList<%s>' % simpleType
    else:
        return simpleType


class OntologyParser():
    def __init__(self):
        self.model = Soprano.createModel()

    def parseFile(self, path):
        parser = Soprano.PluginManager.instance().discoverParserForSerialization(Soprano.SerializationTrig)
        if not parser:
            return False
        it = parser.parseFile(path, QtCore.QUrl("dummy"), Soprano.SerializationTrig)
        while it.next():
            self.model.addStatement(it.current())
        if parser.lastError():
            return False;

        return True

    def writeAll(self):

        # add rdfs:Resource as domain for all properties without a domain
        query = 'select ?p where { ?p a %s . OPTIONAL { ?p %s ?d . } . FILTER(!BOUND(?d)) . }' \
                 % (Soprano.Node.resourceToN3(Soprano.Vocabulary.RDF.Property()), \
                         Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.domain()))
        nodes = self.model.executeQuery(query, Soprano.Query.QueryLanguageSparql).iterateBindings(0).allNodes()
        for p in nodes:
            self.model.addStatement(p, Soprano.Node(Soprano.Vocabulary.RDFS.domain()), Soprano.Node(Soprano.Vocabulary.RDFS.Resource()))
        
        # cache a few values we need more than once
        self.rdfsResourceProperties = self.getPropertiesForClass(Soprano.Vocabulary.RDFS.Resource())

        query = 'select distinct ?uri ?label ?comment where {{ ?uri a {0} . ?uri {1} ?label . OPTIONAL {{ ?uri {2} ?comment . }} . }}' \
            .format(Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.Class()), \
                         Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.label()), \
                         Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.comment()))
        it = self.model.executeQuery(query, Soprano.Query.QueryLanguageSparql)
        
        while it.next():
            uri = it['uri'].uri()
            if verbose:
                print "Parsing class: ", uri
            ns = self.getNamespaceAbbreviationForUri(uri)
            name = extractNameFromUri(uri)
            self.writeHeader(uri, ns, name, it['label'].toString(), it['comment'].toString())
            print "\n\n"

    def getNamespaceAbbreviationForUri(self, uri):
        query = "select ?ns where { graph ?g { %s ?p ?o . } . ?g %s ?ns . } LIMIT 1" \
            % (Soprano.Node.resourceToN3(uri), \
               Soprano.Node.resourceToN3(Soprano.Vocabulary.NAO.hasDefaultNamespaceAbbreviation()))
        it = self.model.executeQuery(query, Soprano.Query.QueryLanguageSparql)
        if it.next():
            return it[0].toString().toLower()
        else:
            return extractOntologyName(uri)

    def getParentClasses(self, uri):
        """
        Returns a dict which maps parent class URIs to a dict containing keys 'ns' and 'name'
        Only parent classes that are actually generated are returned.
        """
        query = "select distinct ?uri where {{ {0} {1} ?uri . ?uri a {2} . }}" \
             .format(Soprano.Node.resourceToN3(uri), \
                          Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.subClassOf()), \
                          Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.Class()))
        it = self.model.executeQuery(query, Soprano.Query.QueryLanguageSparql)
        classes = {}
        while it.next():
            puri = it['uri'].uri()
            if puri != Soprano.Vocabulary.RDFS.Resource():
                cd = {}
                cd['ns'] = self.getNamespaceAbbreviationForUri(puri)
                cd['name'] = extractNameFromUri(puri)
                classes[puri.toString()] = cd
        return classes

    def getFullParentHierarchyTree(self, uri, currentParents):
        """
        Returns a tree with nodes consisting of dicts containing keys 'children', 'ns' and 'name'.
        currentParents is a running variable used to avoid endless loops when recursing. It should
        always be set to the empty list [].
        Only used by getFullParentHierarchy()
        """
        parents = []
        directParents = self.getParentClasses(uri)
        for p in directParents.keys():
            if not p in currentParents:
                currentParents.append(p)
                cd = directParents[p]
                cd['children'] = self.getFullParentHierarchyTree(QtCore.QUrl(p), currentParents)
                parents.append(cd)
        return parents

    def getFullParentHierarchy(self, uri):
        """
        Returns a list of dicts with keys 'ns' and 'name' ordered by reversed specialization.
        This is required for virtual inheritance where the constructors are called beginning
        from most generic one.
        """
        tree = self.getFullParentHierarchyTree(uri, [])
        parents = []
        while len(tree) > 0:
            # Perform a depth-first to find the most general type first
            # the add that type to the list of parents and remove it
            # from the tree.
            tmp = tree[0]
            last = tree
            while len(tmp['children']) > 0:
                last = tmp['children']
                tmp = tmp['children'][0]
            parents.append(tmp)
            last.pop(0)
        return parents

    def getPropertiesForClass(self, uri):
        query = "select distinct ?p ?range ?comment ?c ?mc where { ?p a %s . ?p %s %s . ?p %s ?range . OPTIONAL { ?p %s ?comment . } . OPTIONAL { ?p %s ?c . } . OPTIONAL { ?p %s ?mc . } . }" \
            % (Soprano.Node.resourceToN3(Soprano.Vocabulary.RDF.Property()),
               Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.domain()),
               Soprano.Node.resourceToN3(uri),
               Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.range()),
               Soprano.Node.resourceToN3(Soprano.Vocabulary.RDFS.comment()),
               Soprano.Node.resourceToN3(Soprano.Vocabulary.NRL.cardinality()),
               Soprano.Node.resourceToN3(Soprano.Vocabulary.NRL.maxCardinality()))
        it = self.model.executeQuery(query, Soprano.Query.QueryLanguageSparql)
        #print "Property query done."
        properties = {}
        while it.next():
            p = it['p'].uri()
            r = it['range'].uri()
            comment = it['comment'].toString()
            c = 0
            if it['c'].isValid():
                c = it['c'].literal().toInt()
            else:
                c = it['mc'].literal().toInt()
            properties[p] = dict([('range', r), ('cardinality', c), ('comment', comment)])
        return properties

    def writeComment(self, theFile, text, indent):
        maxLine = 50;

        theFile.write(' ' * indent*4)
        theFile.write("/**\n")
        theFile.write(' ' * (indent*4+1))
        theFile.write("* ")

        words = QtCore.QString(text).split( QtCore.QRegExp("\\s"), QtCore.QString.SkipEmptyParts )
        cnt = 0;
        for i in range(words.count()):
            if cnt >= maxLine:
                theFile.write('\n')
                theFile.write(' ' * (indent*4+1))
                theFile.write("* ")
                cnt = 0;
            theFile.write(words[i])
            theFile.write(' ')
            cnt += words[i].length()

        theFile.write('\n')
        theFile.write(' ' * (indent*4+1))
        theFile.write("*/\n")

    def writeGetter(self, theFile, prop, name, propRange, cardinality):
        theFile.write('    %s %s() const {\n' % (typeString(propRange, cardinality), makeFancy(name, cardinality)))
        theFile.write('        %s value;\n' % typeString(propRange, cardinality))
        if cardinality == 1:
            theFile.write('        if(contains(QUrl::fromEncoded("%s", QUrl::StrictMode)))\n' % prop.toString())
            theFile.write('            value = property(QUrl::fromEncoded("{0}", QUrl::StrictMode)).first().value<{1}>();\n'.format(prop.toString(), typeString(propRange, 1)))
        else:
            theFile.write('        foreach(const QVariant& v, property(QUrl::fromEncoded("%s", QUrl::StrictMode)))\n' % prop.toString())
            theFile.write('            value << v.value<{0}>();\n'.format(typeString(propRange, 1)))
        theFile.write('        return value;\n')
        theFile.write('    }\n')

    def writeSetter(self, theFile, prop, name, propRange, cardinality):
        theFile.write('    void set%s%s(const %s& value) {\n' % (makeFancy(name, cardinality)[0].toUpper(), makeFancy(name, cardinality).mid(1), typeString(propRange, cardinality)))
        theFile.write('        QVariantList values;\n')
        if cardinality == 1:
            theFile.write('        values << value;\n')
        else:
             theFile.write('        foreach(const %s& v, value)\n' % typeString(propRange, 1))
             theFile.write('            values << v;\n')
        theFile.write('        setProperty(QUrl::fromEncoded("%s", QUrl::StrictMode), values);\n' % prop.toString())
        theFile.write('    }\n')

    def writeAdder(self, theFile, prop, name, propRange, cardinality):
        theFile.write('    void add%s%s(const %s& value) {\n' % (makeFancy(name, 1)[0].toUpper(), makeFancy(name, 1).mid(1), typeString(propRange, 1)))
        theFile.write('        addProperty(QUrl::fromEncoded("%s", QUrl::StrictMode), value);\n' % prop.toString())
        theFile.write('    }\n')

    def writeHeader(self, uri, nsAbbr, className, label, comment):
        # Construct paths
        relative_path = nsAbbr + '/' + className.toLower() + '.h'
        folder = output_path + '/' + nsAbbr
        filePath = output_path + '/' + relative_path

        if verbose:
            print "Writing header file: %s" % filePath

        # Create the containing folder
        mkdir_p(QtCore.QFile.encodeName(folder).data())

        # open the header file
        header = open(filePath, 'w')

        # get all direct base classes
        parentClasses = self.getParentClasses(uri)

        # write protecting ifdefs
        header_protect = '_%s_%s_H_' % (nsAbbr.toUpper(), className.toUpper())
        header.write('#ifndef %s\n' % header_protect)
        header.write('#define %s\n' % header_protect)
        header.write('\n')
        
        # write default includes
        header.write('#include <QtCore/QVariant>\n')
        header.write('#include <QtCore/QStringList>\n')
        header.write('#include <QtCore/QUrl>\n')
        header.write('#include <QtCore/QDate>\n')
        header.write('#include <QtCore/QTime>\n')
        header.write('#include <QtCore/QDateTime>\n')
        header.write('\n')

        # all classes need the SimpleResource include
        header.write('#include <nepomuk/simpleresource.h>\n\n')

        # write includes for the parent classes
        parentClassNames = []
        for parent in parentClasses.keys():
            header.write('#include "%s/%s.h"\n' % (parentClasses[parent]['ns'], parentClasses[parent]['name'].toLower()))
            parentClassNames.append("%s::%s" %(parentClasses[parent]['ns'].toUpper(), parentClasses[parent]['name']))

        # get all base classes which we require due to the virtual base class constructor ordering in C++
        # We inverse the order to match the virtual inheritance constructor calling order
        fullParentHierarchyNames = []
        for parent in self.getFullParentHierarchy(uri):
            fullParentHierarchyNames.append("%s::%s" %(parent['ns'].toUpper(), parent['name']))

        if len(parentClassNames) > 0:
            header.write('\n')

        # write the class namespace
        header.write('namespace Nepomuk {\n')
        header.write('namespace %s {\n' % nsAbbr.toUpper())

        # write the class + parent classes
        # We use virtual inheritance when deriving from SimpleResource since our ontologies
        # make use of multi-inheritance and without it the compiler would not know which
        # addProperty and friends to call.
        # We need to do the same with all parent classes since some classes like
        # nco:CellPhoneNumber as derived from other classes that have yet another parent
        # class in common which is not SimpleResource.
        self.writeComment(header, comment, 0)
        header.write('class %s' % className)
        header.write(' : ')
        header.write(', '.join(['public virtual %s' % (p) for p in parentClassNames]))
        if len(parentClassNames) == 0:
            header.write('public virtual Nepomuk::SimpleResource');
        header.write('\n{\n')
        header.write('public:\n')

        # write the default constructor
        # We directly set the type of the class to the SimpleResource. If the class is a base class
        # not derived from any other classes then we set the type directly. Otherwise we use the
        # protected constructor defined below which takes a type as parameter making sure that we
        # only add one type instead of the whole hierarchy
        header.write('    %s()' % className)
        if len(parentClasses) > 0:
            header.write('\n      : ')
        header.write(', '.join([('%s(QUrl::fromEncoded("' + uri.toString().toUtf8().data() + '", QUrl::StrictMode))') % p for p in fullParentHierarchyNames]))
        header.write(' {\n')
        if len(parentClassNames) == 0:
            header.write('        addType(QUrl::fromEncoded("%s", QUrl::StrictMode));\n' % uri.toString())
        header.write('    }\n\n')

        # write the copy constructor
        header.write('    %s(const SimpleResource& res)\n' % className)
        header.write('      : ')
        header.write('SimpleResource(res)')
        if len(parentClassNames) > 0:
            header.write(', ')
            header.write(', '.join([('%s(res, QUrl::fromEncoded("' + uri.toString().toUtf8().data() + '", QUrl::StrictMode))') % p for p in fullParentHierarchyNames]))
        header.write(' {\n')
        if len(parentClassNames) == 0:
            header.write('        addType(QUrl::fromEncoded("%s", QUrl::StrictMode));\n' % uri.toString())
        header.write('    }\n\n')

        # write the assignment operator
        header.write('    %s& operator=(const SimpleResource& res) {\n' % className)
        header.write('        SimpleResource::operator=(res);\n')
        header.write('        addType(QUrl::fromEncoded("%s", QUrl::StrictMode));\n' % uri.toString())
        header.write('        return *this;\n')
        header.write('    }\n\n')

        # Write getter and setter methods for all properties
        # This includes the properties that have domain rdfs:Resource on base classes, ie.
        # those that are not derived from any other class. That way these properties are
        # accessible from all classes.
        properties = self.getPropertiesForClass(uri)
        if len(parentClassNames) == 0:
            properties.update(self.rdfsResourceProperties)

        # There could be properties with the same name - in that case we give the methods a prefix
        for p in properties.keys():
            name = extractNameFromUri(p)
            cnt = 0
            # search for the same name again
            for op in properties.keys():
                if extractNameFromUri(op) == name:
                    cnt+=1
            if cnt > 1:
                name = self.getNamespaceAbbreviationForUri(p).toLower() + name[0].toUpper() + name.mid(1)
            properties[p]['name'] = name;
            
        for p in properties.keys():
            self.writeComment(header, 'Get property %s. %s' % (p.toString(), properties[p]['comment']), 1)
            self.writeGetter(header, p, properties[p]['name'], properties[p]['range'], properties[p]['cardinality'])
            header.write('\n')
            self.writeComment(header, 'Set property %s. %s' % (p.toString(), properties[p]['comment']), 1)
            self.writeSetter(header, p, properties[p]['name'], properties[p]['range'], properties[p]['cardinality'])
            header.write('\n')
            self.writeComment(header, 'Add value to property %s. %s' % (p.toString(), properties[p]['comment']), 1)
            self.writeAdder(header, p, properties[p]['name'], properties[p]['range'], properties[p]['cardinality'])
            header.write('\n')

        # write the protected constructors which avoid adding the whole type hierarchy
        header.write('protected:\n')
        header.write('    %s(const QUrl& type)' % className)
        if len(parentClassNames) > 0:
            header.write('\n      : ')
            header.write(', '.join(['%s(type)' % p for p in fullParentHierarchyNames]))
        header.write(' {\n')
        if len(parentClassNames) == 0:
            header.write('        addType(type);\n')
        header.write('    }\n')

        header.write('    %s(const SimpleResource& res, const QUrl& type)\n' % className)
        header.write('      : ')
        header.write('SimpleResource(res)')
        if len(parentClassNames) > 0:
            header.write(', ')
            header.write(', '.join(['%s(res, type)' % p for p in fullParentHierarchyNames]))
        header.write(' {\n')
        if len(parentClassNames) == 0:
            header.write('        addType(type);\n')
        header.write('    }\n')

        # close the class
        header.write('};\n')

        # write the closing parenthesis for the namespaces
        header.write('}\n}\n')

        # write the closing preprocessor thingi
        header.write('\n#endif\n')
        

def main():
    global output_path
    global verbose
    
    usage = "Usage: %prog [options] ontologyfile1 ontologyfile2 ..."
    optparser = argparse.ArgumentParser(description="Nepomuk SimpleResource code generator. It will generate a hierarchy of simple wrapper classes around Nepomuk::SimpleResource which provide convinience methods to get and set properties of those classes. Each wrapper class will be defined in its own header file and be written to a subdirectory named as the default ontology prefix. Example: the header file for nao:Tag would be written to nao/tag.h and be defined in the namespace Nepomuk::NAO.")
    optparser.add_argument('--output', '-o', type=str, nargs=1, metavar='PATH', dest='output', help='The destination folder')
    optparser.add_argument('--quiet', '-q', action="store_false", dest="verbose", default=True, help="don't print status messages to stdout")
    optparser.add_argument("ontologies", type=str, nargs='+', metavar="ONTOLOGY", help="Ontology files to use")

    args = optparser.parse_args()
    if args.output :
        output_path = args.output[0]

    verbose = args.verbose

    if verbose:
        print 'Generating from ontology files %s' % ','.join(args.ontologies)
        print 'Writing files to %s.' % output_path

    # Parse all ontology files
    ontoParser = OntologyParser()
    for f in args.ontologies:
        if verbose:
            print "Reading ontology '%s'" % f
        ontoParser.parseFile(f)
    if verbose:
        print "All ontologies read. Generating code..."

    # Get all classes and handle them one by one
    ontoParser.writeAll()

if __name__ == "__main__":
    main()
