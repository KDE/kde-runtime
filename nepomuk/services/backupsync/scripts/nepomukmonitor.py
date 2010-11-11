#!/usr/bin/python
# -*- coding: utf-8 -*-

import psutil
import subprocess
import os
import sys
import getopt

def inMB( item ) :
  return "%.4f" % (float(item) / 1024.0 / 1024.0)
  
def getProcesses() :
    plist = []
    pidlist = psutil.get_pid_list()
    for pid in pidlist :
        p = psutil.Process( pid )
        name = ""
        if "nepomuk" in p.name :
            name = p.cmdline[-1]
            name = name[ len("nepomuk") : ]
        elif  "virtuoso" in p.name :
            name = p.name[: len(p.name) -2 ]
        
        if name :
            rss, vms =  p.get_memory_info()
            rss = inMB( rss )
            process = Process()
            process.name = name
            process.pid = p.pid
            process.mem = rss
            process.mem_per = p.get_memory_percent()
            process.cpu = p.get_cpu_percent()
            
            plist.append( process )
    return plist
    
        
class Process :
    name = ""
    pid = ""
    mem = ""
    mem_per = ""
    cpu = ""
    
scriptName = "/tmp/nepomuk-script.sh"
def generate_script() :
    file = open( scriptName, "w" )
    contents = """#!/bin/bash

echo "attach $1" > conf
echo "thread apply all backtrace" >> conf
echo "detach" >> conf

echo "$2"
gdb --batch -x conf
echo "============================="

rm conf"""
    file.write( contents )
    file.close()

if __name__ == '__main__':

    # Command line options
    long_options = [ "--cpu=", "--memory=" ]
    try :
        optList, args = getopt.getopt( sys.argv[1:], "c:m:", long_options )
    except getopt.GetoptError as err:
        print(str(err))
        print("Usage : --cpu minUsage --mem minMemory ")
        print("Usage : -c minCpuUsage -m minMemoryUsage")
        
    cpu = 0
    mem = 0

    for o, v in optList :
        if o in ["-c", "--cpu" ] :
            cpu = v
        if o in ["-m", "--memory" ] :
            mem = v

    
    plist = getProcesses()
    generate_script()
    
    # Check if any is above normal cpu usage
    for p in plist :
        if p.cpu >= cpu or p.mem_per >= mem :
            print("\t",p.name)
            print("Cpu :", p.cpu, "%")
            print("Mem :", p.mem_per, "%")
            print("Mem :", inMB(p.mem), "Mb")
            subprocess.call( ["bash", str(scriptName), str(p.pid), str(p.name) ] )
            print("\n")

    os.remove( scriptName )