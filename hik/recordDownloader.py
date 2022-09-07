#!/usr/bin/python

import sys
import httplib
import base64
import re
import time

_username = None
_password = None
_host = None

def usage():
	print '''
usage: {app} <user>:<pwd>@<host> <record-playback-url> <saved-file>
example: {app} admin:admin123@10.69.3.129 'rtsp://10.69.3.129/Streaming/tracks/101?starttime=2015-10-15T07:26:02Z&amp;endtime=2015-10-15T07:29:00Z&amp;name=ch01_00000000004000300&amp;size=36323788' download.mp4
notes: <record-playback-url> is the url which can be got by running recordFinder.py 
	'''.format(app=sys.argv[0])
	sys.exit(-1)

def log(msg):
	t = time.time()
	timeStr = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(t))
	timeStr += ".%3d" % (1000*(t - int(t)))
	print timeStr + " " + msg

def httpRequest(method, uri, body=None):
	c = httplib.HTTPConnection(_host)
	usrpwd = _username + ":" + _password
	headers = {
		"Authorization": 'Basic %s' %  base64.b64encode(usrpwd).decode("ascii"),
		"Connection": "keep-alive"
	}
	if body != None:
		headers["Content-Type"] = "applicaton/xml"
		headers["Content-Length"] = str(len(body))
	c.request(method, uri, body, headers)
	r = c.getresponse()
	return r

def download(playbackURI, filename):
	body = '''
	<?xml version="1.0"?>
	<downloadRequest>
		<playbackURI>%s</playbackURI>
	</downloadRequest>
	''' % playbackURI 
	r = httpRequest("GET", "/ISAPI/ContentMgmt/download", body)
	if r.status != 200:
		log("Download failed. [%s]" % r.reason)
		sys.exit(-1)
	f = open(filename, 'w')
	dowloaded = 0;
	log("Download started.")
	while True:
		bytes = r.read(8192)
		if not bytes or len(bytes) == 0:
			print "\rDownloaded: %d bytes." % dowloaded
			log("Download completed.")
			f.close()
			break
		else:
			f.write(bytes)
			dowloaded += len(bytes)
			print "\rDownloaded: %.1f M" % (dowloaded/(1024.0*1024.0)),

if __name__ == "__main__":
	if len(sys.argv) != 4:
		usage()
	m1 = re.compile("(\w+):(\w+)@(.*)").match(sys.argv[1])
	if m1 == None:
		usage()
	_username= m1.group(1)
	_password = m1.group(2)
	_host = m1.group(3)
	playbackURI = sys.argv[2]
	filename = sys.argv[3]
	download(playbackURI, filename)

