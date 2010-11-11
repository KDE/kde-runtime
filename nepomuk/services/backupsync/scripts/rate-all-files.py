#!/usr/bin/python
# -*- coding: utf-8 -*-

# This script looks for all the files mentioned in the directory provided as the argument, and annotates all the files present with random ratings. If a file is not provided as an argument, it annotates all the files/folders in the local directory

import sys
import os
import glob
import random

from PyKDE4.nepomuk import Nepomuk
from PyKDE4.soprano import Soprano
from PyQt4.QtCore import QUrl

nieurl = QUrl("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#url")

path = ""
if len( sys.argv ) < 2 :
	path = os.getcwd()
else :
	path = argv[1]
	
if not path.endswith('/') :
	path += '/'
	
files = glob.glob( path + "*" )
files.sort()

for file in files :
	if not file :
		continue
	
	res = Nepomuk.Resource( file )
	rating = random.randint( 1, 10 )
	res.setRating( rating )
	print "Setting ", res.property(nieurl).toString(), rating

print "\nDone!"

