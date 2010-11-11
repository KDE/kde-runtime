#!/usr/bin/python

import sys
import os
import glob
import shutil

#
# Check number of arguments and print help message
#
print(sys.argv)

if len(sys.argv) < 2 :
    print("Correct usage: ", sys.argv[0], " list/backup <name>/restore <name>")
    quit()

argv = sys.argv[1:]
command = argv[0]

base_dir = os.getenv("KDEHOME") + "/share/"
repo_dir = base_dir + "apps/nepomuk/repository"
config_dir = base_dir + "config/";
storage_dir = base_dir + "apps/nepomuk/backupscript/"

os.mkdir( storage_dir )

if command == "list"  :
    tags = glob.glob( storage_dir + "*" )
    print(tags)


elif command == "backup" :
    tag = argv[1]
    path = storage_dir + tag;
    shutil.rmtree( path )
    os.mkdir( path )

    tagDir = path + '/'
    
    # Copy the repo
    shutil.cptree( repo_dir, tagdir )
    shutil.copyfile( config_dir + "nepomukserverrc", tagDir + "nepomukserverrc" );
    shutil.copyfile( config_dir + "nepomukstrigirc", tagDir + "nepomukstrigirc" );
    shutil.copyfile( config_dir + "kio_nepomuksearchrc", tagDir + "kio_nepomuksearchrc" );

    print("Done!")

elif command == "restore" :
    tag = argv[1]
    path = storage_dir + tag;

    shutil.rmtree( repo_dir )

    shutil.cptree( tagdir + "/repository", repo_dir )
    
    shutil.copyfile( tagDir + "nepomukserverrc", config_dir + "nepomukserverrc" );
    shutil.copyfile( tagDir + "nepomukstrigirc", config_dir + "nepomukstrigirc" );
    shutil.copyfile( tagDir + "kio_nepomuksearchrc", config_dir + "kio_nepomuksearchrc" );

    print("Restored!")
