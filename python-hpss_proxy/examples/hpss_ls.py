#!/usr/bin/python

import pprint

from hpss_proxy import HPSS_PROXY

url = "127.0.0.1:8080"
LogFile = None

pp = pprint.PrettyPrinter()

proxy = HPSS_PROXY(url, LogFile)
pp.pprint(proxy.hpss_ls("/tests", flags="l", depth=2))

