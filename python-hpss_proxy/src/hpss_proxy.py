"""@package HPSS_PROXY
Translate python speak into hpss_http_proxy speak.
"""
import httplib
import json
import os
import sys
import urllib
from logging import DEBUG, INFO, WARN, ERROR, CRITICAL

class HPSS_PROXY:
   
    MAX_RETRY_COUNT = 3
    
    def __init__(self, url, LogFile=None, LogLevel=INFO) :
        """ Initializer. Set up http-connection and logging. 

            Do not use the python logging module, but just write into a simple
            File to avoid multithreading.
            The LogLevels are taken from the logging-module, however
        """

         
        if LogFile == None :
            self.LogFile = file("/dev/null", "w")
        else :
            self.LogFile = LogFile
        self.LogLevel = LogLevel
        self.url = url 
        self.conn = httplib.HTTPConnection(self.url)
        self.retry_count = 0
	

    def check_error(self, result_obj) :
        """ return the actual error from a result_obj   """
        if "errno" in result_obj.keys() :
            return (result_obj["errno"], result_obj["errstr"])
        else :
            return (0, "")

    def walk(self, top, topdown=True, onerror=None, followlinks=False):
        """ copy & paste from os.walk.
        
        For each directory in the directory tree rooted at top (including top
        itself, but excluding '.' and '..'), yields a 3-tuple
        dirpath, dirnames, filenames
        XXX should be extended to use filter options available in hpss_ls.
        """
        try:
            names = self.hpss_ls(top, flags="l", depth=1 )
        except :
            if onerror is not None:
                onerror(err)
            return

        dirs, nondirs = [], []
        if self.LogLevel <= DEBUG :
            self.LogFile.write("XXX walk: names = %s\n" % ( names ) )
        for name in names["contents"]:
            full_path = os.path.join(top, name["path"])
            if self.isdir(full_path):
                dirs.append(full_path)
            else:
                nondirs.append(name["path"])
    
        if topdown:
            yield top, dirs, nondirs

        for name in dirs:
            full_path = (os.path.join(top, name))
            if followlinks or not self.islink(full_path):
                for x in self.walk(full_path, topdown, onerror, followlinks):
                    yield x

        if not topdown:
            if self.LogLevel <= DEBUG :
                self.LogFile.write("XXX yielding: %s %s %s\n" % ( top, dirs, nondirs  ) )
            yield top, dirs, nondirs

    def isdir(self, path) :
        """ hpss-analogous to os.path.isdir. 

            return boolean if path in HPSS is a dir or not.
        """
        result_obj = self.hpss_ls(path, flags="l", depth="0") 
        if self.LogLevel <= DEBUG :
            self.LogFile.write("XXX isdir: path=%s\n" % (path))
        if result_obj["attributes"]["type"]  == "d" :
            return True
        else :
            return False
            
    def islink(self, path) :
        """ hpss-analogous to os.path.islink. 

            return boolean if path in HPSS is a symbolic link or not.
        """
        result_obj = self.hpss_ls(path, flags="l", depth="0") 
        if result_obj["attributes"]["type"]  == "l" :
            return True
        else :
            return False

    def do_action(self, action, path, flags="", depth=0, older=-1, newer=-1, extra_params={} ) :
        """ actually call the hpss_http_proxy and load the returned json into a result_obj.

            throw an error on a http code != 200.
        """
        param_dict = {'action': action, 'flags': flags, 'depth': depth, "older" : "%s" % older, "newer": "%s" % newer } 
        param_dict.update(extra_params) 
        if self.LogLevel <= DEBUG :
            self.LogFile.write("path=%s, param_dict: %s\n" % (path, param_dict))
        if not param_dict["flags"] :
            param_dict.pop("flags")
        params = urllib.urlencode( param_dict)
        if self.LogLevel <= DEBUG :
            self.LogFile.write("sending: GET %s?%s" % ( path, params))
        while self.retry_count < self.MAX_RETRY_COUNT :
            try :
                self.conn.request("GET", "%s?%s" % (path, params) )
                response = self.conn.getresponse()
                break
            except httplib.BadStatusLine :
                self.retry_count += 1
                self.conn = httplib.HTTPConnection(self.url)

        if self.retry_count > self.MAX_RETRY_COUNT :
            if self.LogLevel <= ERROR :
                 self.LogFile.write("Cannot connect to hpss-http-proxy")
            raise ("Cannot connect to hpss-http-proxy")
        else :
            self.retry_count = 0
 
        if response.status != 200 :
            if self.LogLevel <= ERROR :
                self.LogFile.write("got response (%d, %s) : %s" % (response.status, response.reason, response.read()))
            raise RuntimeError("got response (%d, %s) : %s" % (response.status, response.reason, response.read()))
        data = response.read()
        if self.LogLevel <= DEBUG :
            self.LogFile.write("do_action: action=%s : proxy returned data: %s\n" % (action,data))
            self.LogFile.flush()
        result_obj=json.loads( data)
        return result_obj 

    def hpss_link(self, path, flags, dest_path) :
        """ create a link within hpss from path to dest_path.

            use flag "h" for a hardlink, otherwise a softlink 
            is created.
        """
        return self.do_action("link", path, flags=flags, extra_params={ "dest_path" : dest_path})

    def hpss_ls(self, path, flags="", depth=0, older=-1, newer=-1) :
        """ list dir at path. 

            depth is the number of levels to recurse into. older and newer maybe used
            to filter results.
        """
        return self.do_action("ls", path, flags=flags, depth=depth, older=older, newer=newer)

    def hpss_mkdir(self, path, flags="") :
        """ create a dir within hpss at path.

            use flag "p" to create also non-existing
            parent dirs.
            XXX mode_str should be passed along. 
        """
        return self.do_action("mkdir", path, flags=flags)

    def hpss_rm(self, path, flags="") :
        """ remove path recursively. 

            use flag "r" for recursion.
        """
        if "r" in flags :
            for root, dirs, files in self.walk(path, topdown=False) :
                sys.stderr.write("XXX root=%s, dirs=%s, files=%s\n" % (root,dirs, files) )
                for f in files :
                    sys.stderr.write("XXX removing file: %s\n" % os.path.join(root,f) )
                    self.do_action("rm", os.path.join(root, f), "")   
                for d in dirs :
                    sys.stderr.write("XXX removing dir: %s\n" % os.path.join(root, d) )
                    self.do_action("rm", os.path.join(root, d), "r")   
            sys.stderr.write("XXX removing dir: %s\n" % root)   
            return self.do_action("rm", root, "r")   
        else :
            return self.do_action("rm", path, flags=flags)

    def hpss_put_from_proxy(self, path, local_path ) :
        """ put a file local to this proxy into hpss.

            XXX mode_str ?, recursion ?
        """
        return self.do_action("put_from_proxy", path, extra_params = { "local_path": local_path} );

    def hpss_get_to_proxy(self, path, flags, local_path ) :
        """ get a file from hpss and store it locally.

            XXX mode_str ?, recursion ?
        """

        return self.do_action("get_to_proxy", path, flags=flags, extra_params = { "local_path": local_path} );

    def hpss_chmod(self, path, flags, mode_str) :
        """ do a chmod on path. mode_str is of form "rwxrwx---"

            use flag "r" for recursion.
        """
        if "r" in flags :
            for root, dirs, files in self.walk(path, topdown=True) :
                for f in files + dirs :
                    self.do_action("chmod", os.path.join(root,f), extra_params = { "mode_str": mode_str} );
            self.do_action("chmod", root, extra_params = { "mode_str": mode_str} );
        return self.do_action("chmod", path, extra_params = { "mode_str": mode_str} );

    def hpss_rename(self, path, flags, new_path) :
        """ rename an object.

            no extra flags etc. implemented.
        """
        return self.do_action("rename", path, flags=flags, extra_params={ "new_path" : new_path})

    def hpss_stat(self, path, flags) :
        """ return stat information from a file.

            no recursion etc. implemented.
        """
        return self.do_action("stat", path, flags=flags);

    def hpss_stage(self, path, storage_level=0) :
        """ stage a file. By default to storage-level 0 (disk).

            no recursion etc. implemented.
        """
        return self.do_action("stage", path, extra_params={"storage_level": storage_level})

    def hpss_get_storage_info(self, path) :
        """ return id of physical volume
 
            no recursion etc. implemented.
        """
        return self.do_action("get_storage_info", path);

    def hpss_chown(self, path, flags, uid, gid) :
        """ change ownership of a file or directory.

            no recursion etc. implemented.
        """
        return self.do_action("chown", path, flags=flags, extra_params={"uid" : uid, "gid" : gid}) 

    def hpss_get_udas(self, path, flags ) :
        """ get full uda-list.

            no recursion etc. implemented.
        """
        return self.do_action("get_udas", path, flags=flags )

    def hpss_set_uda(self, path, flags, uda_list ) :
        """ set uda-list. uda-list is acutally a dict. Each item will be set.

            no recursion etc. implemented.
        """
        for key, value in uda_list.iteritems() :
            res = self.do_action("set_uda", path, flags=flags, extra_params={"key": key, "value" : value})
            rc, msg = self.check_error(res)
            if rc :
                return res
        return res
 
    def hpss_del_uda(self, path, flags, key ) :
        """ delete a uda from the  uda-list.
        
            no recursion etc. implemented.
        """
        return self.do_action("del_uda", path, flags=flags, extra_params={"key": key} )
