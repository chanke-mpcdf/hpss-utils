#!/usr/bin/python
import argparse
import os

from hpss_proxy import HPSS_PROXY

ARGPARSER = argparse.ArgumentParser()
ARGPARSER.add_argument("--hpss_dir", required=True, help = "dir in HPSS for testing. will be removed after test.")
ARGPARSER.add_argument("--url", required=True, help = "url of hpss_http_proxy like 127.0.0.1:8080")
ARGPARSER.add_argument("--local_file", required=True, help = "local_file to put")
ARGPARSER.add_argument("--log_file", default="/dev/null", help = "path to logfile storing extended reports")

args=ARGPARSER.parse_args()

proxy = HPSS_PROXY(args.url)

print  proxy.hpss_put_from_proxy("%s/%s" % (args.hpss_dir, os.path.basename(args.local_file)), args.local_file) 
