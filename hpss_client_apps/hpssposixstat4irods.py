#!/usr/bin/python

import time,string,subprocess,sys,getopt


try:
    opts, args = getopt.getopt(sys.argv[1:], "u:t:", [])
except getopt.GetoptError, err:
    # print help information and exit:
    print str(err) # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

Keytab=""
Username=""

for o, a in opts:
    if o == "-u":
        Username = a
    elif o in ("-t"):
        Keytab=a 
    else:
        assert False, "unhandled option"

if Username == "" or Keytab == "" :
    sys.stderr.write("Both -u <username> and -t <path to keytab required.>\n")
    sys.exit(1)

if len(args) != 1 :
    sys.stderr.write("need filename as one and only arg.\n")
    sys.exit(1)
filename=args[0]

subproc=subprocess.Popen(["/opt/hpss/examples/hpss_posixstat", "-u", "%s" % Username, "-k", "-t", "%s" % Keytab, "%s" % filename],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
(stdoutdata, stderrdata)=subproc.communicate()

if subproc.returncode != 0 :
    sys.stderr.write(stderrdata)
    sys.exit(subproc.returncode)

Month={"Jan" : 1, "Feb" : 2, "Mar" : 3 ,"Apr" : 4, "May": 5, "Jun" :6 ,"Jul" : 7, "Aug": 8, "Sep" : 9 ,"Oct": 10, "Nov" : 11, "Dec" : 12 }

for line in stdoutdata.split("\n") :
    line=line.strip()
    if not line : continue
    tokens=line.split()
    if tokens[0] == "device" :
        device = tokens[2]
    elif tokens[0] == "inode" :
        inode = tokens[2]
    elif tokens[0] == "mode" :
        mode = tokens[2]
    elif tokens[0] == "links" :
        nlink = tokens[2]
    elif tokens[0] == "uid" :
        uid = tokens[2]
    elif tokens[0] == "gid" :
        gid = tokens[2]
    elif tokens[0] == "dev" and tokens[1] == "type" :
        devid = tokens[3]
    elif tokens[0] == "size" :
        size = tokens[2]
    elif tokens[0] == "blksize" :
        blksize = tokens[2]
    elif tokens[0] == "blocks" :
        blkcnt = tokens[2]
    elif tokens[0] == "access" and tokens[1] == "time" :
        atime = "%s-%s-%s-%s" % (tokens[7],Month[tokens[4]],tokens[5],tokens[6].replace(":","."))
    elif tokens[0] == "modify" and tokens[1] == "time" :
        mtime = "%s-%s-%s-%s" % (tokens[7],Month[tokens[4]],tokens[5],tokens[6].replace(":","."))
    elif tokens[0] == "change" and tokens[1] == "time" :
        ctime = "%s-%s-%s-%s" % (tokens[7],Month[tokens[4]],tokens[5],tokens[6].replace(":","."))
try :
   print "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s:%s"% (device,inode,mode,nlink,uid,gid,devid,size,blksize,blkcnt,atime,mtime,ctime)
except :
   sys.exit(-1)

