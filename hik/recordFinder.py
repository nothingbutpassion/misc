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
usage: {app} <user>:<pwd>@<host> <start-time> <end-time>
example: {app} admin:admin123@10.69.3.129 2015-10-15T00:00:00Z 2015-10-15T23:00:00Z
notes:	<start-time> and <end-time> must be UTC time like the above example
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

def search(startTime, endTime):
	body = '''
	<?xml version="1.0" encoding="utf-8"?>
	<CMSearchDescription>
		<searchID>C6C85C87-58F0-0001-B689-127019008710</searchID>
		<trackIDList>
			<trackID>101</trackID>
		</trackIDList>
		<timeSpanList>
			<timeSpan>
				<startTime>%s</startTime>
				<endTime>%s</endTime>
			</timeSpan>
		</timeSpanList>
		<maxResults>400</maxResults>
		<searchResultPostion>0</searchResultPostion>
		<metadataList>
			<metadataDescriptor>//recordType.meta.std-cgi.com/VideoMotion</metadataDescriptor>
		</metadataList>
	</CMSearchDescription>
	''' % (startTime, endTime)
	r = httpRequest("POST", "/ISAPI/ContentMgmt/search", body)
	if r.status != 200:
		log("Search record failed. [%s]" % r.reason)
		sys.exit(-1)
	bytes = r.read()
	ms = re.compile('<playbackURI>([^<]+)</playbackURI>').finditer(bytes)
	for m in ms:
		print m.group(1)

if __name__ == "__main__":
	if len(sys.argv) != 4:
		usage()
	m1 = re.compile("(\w+):(\w+)@(.*)").match(sys.argv[1])
	m2 = re.compile("(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)").match(sys.argv[2])
	m3 = re.compile("(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)").match(sys.argv[3])
	if m1 == None or m2 == None or m3 == None:
		usage()
	_username= m1.group(1)
	_password = m1.group(2)
	_host = m1.group(3)
	startTime = m2.group(1)
	endTime = m3.group(1)
	search(startTime, endTime)


