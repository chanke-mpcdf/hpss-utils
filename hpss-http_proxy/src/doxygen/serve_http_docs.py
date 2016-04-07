#!/usr/bin/python

import os
import SimpleHTTPServer
import SocketServer
import socket


PORT = 8000
DOC_DIR = "docs/html/"
Handler = SimpleHTTPServer.SimpleHTTPRequestHandler
os.chdir(DOC_DIR)

while 1 :
    try :
        httpd = SocketServer.TCPServer(("", PORT), Handler)
        break
    except socket.error :
        PORT += 1

print "serving at port", PORT
httpd.serve_forever()
