#!/usr/bin/python

import argparse
import inspect
import os
import sys
import time
import unittest
from hpss_proxy import HPSS_PROXY

global headers, hpss_test_dir, hpss_test_file, local_test_dir, local_test_file, url, is_privileged, LogFile, force, keep_hpss_files

class TestHTTP_PROXY(unittest.TestCase):
    global hpss_test_dir, hpss_test_file, local_test_dir, local_test_file, url, is_privileged, LogFile, force, keep_hpss_files

    @classmethod
    def setUpClass(self):
        LogFile.write("is_privileged=%s\n\n" % is_privileged)
        LogFile.write("setting up test-dir in HPSS...\n\n")
        res =  proxy.hpss_ls(hpss_test_dir, depth="1")
        if proxy.check_error(res)[0] == 0 :
            if force :
                res = proxy.hpss_rm(hpss_test_dir, flags="r")
                if proxy.check_error(res)[0] :
                    raise RuntimeError("Could not delete existing directory %s." % res)
        res = proxy.hpss_mkdir(hpss_test_dir)
        if proxy.check_error(res)[0]  :
             raise RuntimeError("Cannot create dir: %s" % res)
        res = proxy.hpss_put_from_proxy("%s/%s" % (hpss_test_dir, hpss_test_file), "%s/%s" % (local_test_dir, local_test_file) )
        if proxy.check_error(res)[0] :
             raise RuntimeError("Cannot upload local file: %s" % res)
        res = proxy.hpss_put_from_proxy("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), "%s/%s" % (local_test_dir, local_test_file) )
        if proxy.check_error(res)[0] :
             raise RuntimeError("Cannot upload local file: %s" % res)

    @classmethod
    def tearDownClass(self) :
        if keep_hpss_files :
            LogFile.write("Keeping all created stuff in HPSS under %s\n" % hpss_test_dir)
        else :
            LogFile.write("tearing down test-dir in HPSS...\n\n")
            res = proxy.hpss_rm(hpss_test_dir, flags="r")
            if proxy.check_error(res)[0] :
                sys.stderr.write("WARNING! Could not delete test-directory %s in HPSS: %s\n" % res)

    def test_single_mkdir(self):
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        new_dir="hulla"
        res = proxy.hpss_mkdir("%s/%s" % (hpss_test_dir, new_dir), {})
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_mkdir_hier(self):
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        new_dirs="hallo/welt"
        res = proxy.hpss_mkdir("%s/%s" % (hpss_test_dir, new_dirs), {"flags": "p"})
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_hard_link(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_link("%s/%s" % (hpss_test_dir, hpss_test_file) , "h", "%s/%s-hard" % (hpss_test_dir, hpss_test_file)  )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_sym_link(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_link("%s/%s" % (hpss_test_dir, hpss_test_file) ,"s", "%s/%s-sym" % (hpss_test_dir, hpss_test_file) )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
   
    def test_chown(self) :
        if not is_privileged :
            LogFile.write("skipping %s...\n" % inspect.stack()[0][3])
            unittest.skip("rpc-server not running as privileged user")
            return
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_chown("%s/%s" % (hpss_test_dir, hpss_test_file) ,{}, 63232, 63232)
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
       
    def test_chmod(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_chmod("%s/%s" % (hpss_test_dir, hpss_test_file), {},  "rwxr-xr-x")
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
       

    def test_ls(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_ls("%s" % hpss_test_dir, {}) 
        LogFile.write("res=%s\n\n" % res)
        self.assertTrue(len(res["path"]) >= 1)

    def test_ls_recursive(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_ls("%s" % hpss_test_dir, {"flags" :"raX"})
        LogFile.write("res=%s\n\n" % res)
        self.assertTrue(len(res["path"]) >= 1)

    def test_rename(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_rename("%s/%s" % (hpss_test_dir, hpss_test_file) ,{}, "%s/%s-rename" % (hpss_test_dir, hpss_test_file))
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
        # undo the rename
        res = proxy.hpss_rename("%s/%s-rename" % (hpss_test_dir, hpss_test_file) ,{}, "%s/%s" % (hpss_test_dir, hpss_test_file))

    def test_create_rm(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_link("%s/%s" % (hpss_test_dir, hpss_test_file) ,{"flags" : "h"}, "%s/%s-to_be_deleted" % (hpss_test_dir, hpss_test_file)  )
        res = proxy.hpss_rm("%s/%s-to_be_deleted" % (hpss_test_dir, hpss_test_file), {})
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_create_rmdir(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        new_dirs="to_be_deleted/welt"
        res_parents = proxy.hpss_mkdir("%s/%s" % (hpss_test_dir, new_dirs), {"flags": "p"})
        res = proxy.hpss_rm("%s/to_be_deleted" % (hpss_test_dir), {"flags" :"r"})
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res_parents)[1], "")

    def test_stat(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_stat("%s" % (hpss_test_dir), {"flags" : "r"})
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_get_storage_info(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_get_storage_info("/afs/ipp-garching.mpg.de/backups/tests/local_testi")
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_put_from_get_to_proxy(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_put_from_proxy("%s/%s-put-get" % (hpss_test_dir, hpss_test_file), "/%s/%s" % (local_test_dir, local_test_file) )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
        res = proxy.hpss_get_to_proxy("%s/%s-put-get" % (hpss_test_dir, hpss_test_file) , "f",  "/%s/%s" % (local_test_dir, local_test_file) )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    def test_set_get_del_get_uda(self) :
        udas={"/tests/first" : "last", "/tests/last" : "first"}
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_set_uda("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {}, udas )
        LogFile.write("set_uda: res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
        res = proxy.hpss_get_uda("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {} )
        LogFile.write("get_uda: res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
        res = proxy.hpss_del_uda("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {}, "/tests/last" )
        LogFile.write("del_uda: res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")
        res = proxy.hpss_get_uda("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {} )
        LogFile.write("get_uda: res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    @unittest.expectedFailure
    def test_migrate(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_migrate("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {}, 1 )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

    @unittest.expectedFailure
    def test_purge_stage(self) :
        LogFile.write("testing %s...\n" % inspect.stack()[0][3])
        res = proxy.hpss_purge("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {} )
        LogFile.write("res=%s\n\n" % res)
        res = proxy.hpss_stage("%s/%s-pinned" % (hpss_test_dir, hpss_test_file), {} )
        LogFile.write("res=%s\n\n" % res)
        self.assertEqual(proxy.check_error(res)[1], "")

if __name__ == '__main__':
    ARGPARSER = argparse.ArgumentParser()
    ARGPARSER.add_argument("--local_file", default="local_testi", help = "local file for upload into hpss")
    ARGPARSER.add_argument("--local_dir", required=True, help = "local directory file for download from hpss")
    ARGPARSER.add_argument("--hpss_file", default="testi", help = "filename inside hpss")
    ARGPARSER.add_argument("--hpss_dir", required=True, help = "dir in HPSS for testing. will be removed after test.")
    ARGPARSER.add_argument("--url", required=True, help = "url of hpss_http_proxy like 127.0.0.1:8080")
    ARGPARSER.add_argument("--hpss_admin", action='store_true', help = "hpss_http-proxy runs as privilegded user")
    ARGPARSER.add_argument("--force", action='store_true', help = "delete old HPSS-dir if it exists.")
    ARGPARSER.add_argument("--log_file", default="/dev/null", help = "path to logfile storing extended reports")
    ARGPARSER.add_argument("--keep_hpss_files", action="store_true", help = "keep all stuff created in HPSS")
    ARGPARSER.add_argument('unittest_args', nargs='*')
     
    args=ARGPARSER.parse_args()
    
    local_test_file=args.local_file
    local_test_dir=args.local_dir
    hpss_test_file=args.hpss_file
    hpss_test_dir=args.hpss_dir
    url=args.url
    is_privileged=args.hpss_admin
    LogFile=file(args.log_file,"w+")
    force=args.force
    keep_hpss_files = args.keep_hpss_files
    
    # thx to tghw at 
    # http://stackoverflow.com/questions/1029891/python-unittest-is-there-a-way-to-pass-command-line-options-to-the-app
    # for the unittest_args trick.
    sys.argv[1:] = args.unittest_args

    if not os.path.exists("%s/%s" % (local_test_dir,local_test_file)) :
        f=file("%s/%s" % (local_test_dir,local_test_file) ,"w+")
        f.write("Hallo Welt!\n")
        f.close()
    else :
        if not os.path.isfile("%s/%s" % (local_test_dir,local_test_file)) :
            raise RuntimeError("local testfile %s/%s is either autocreated by me or must be atrue file" % (local_test_dir,local_test_file))

    proxy = HPSS_PROXY(url, LogFile)
     
    unittest.main()

