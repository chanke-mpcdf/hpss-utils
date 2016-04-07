#!/usr/bin/python

from hpss_proxy import HPSS_PROXY


wanted_mod="rxwr-x---"
wanted_gid=63232
wanted_uid=502


LogFile=file("./fix_gid_perm", "w+")
proxy = HPSS_PROXY("127.0.0.1:8080", LogFile)
path="/afs/ipp-garching.mpg.de/dir1/AFSIDat/I=/Ilu20=/+/=" 
print proxy.hpss_ls(path, flags="la")
print proxy.hpss_chown("%s" % (path) ,{}, 502, 63232)
print proxy.hpss_ls(path, flags="la")



