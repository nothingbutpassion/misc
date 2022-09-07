#!/usr/bin/python

import sys
import httplib
import base64
import time
import re

_host = None
_username = None
_password = None

def usage():
	print """
	usage: {app} <username>:<password>@<host> <duration-minutes>
	examples: {app} admin:admin123@10.69.3.129 5
	""".format(app=sys.argv[0])
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

def	deviceLocalTime():
	r = httpRequest("GET", "/ISAPI/System/time/localTime")
	if r.status == 200:
		bytes = r.read()
		#localTime format: 2015-10-13T15:35:53+08:00
		p = re.compile("(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})[+-]\d{2}:\d{2}")
		m = p.search(bytes)
		localTime = time.strptime(m.group(1), "%Y-%m-%dT%H:%M:%S")
		#log("Get device localtime succeed. [%s]" % time.strftime("%Y-%m-%d %H:%M:%S", localTime))
		return time.mktime(localTime)
	else:
		log("Get deivce localtime failed. [%s]" % r.reason)
		sys.exit(-1)


def record(dayOfWeek, startTime, endTime):
	body = """
	<?xml version="1.0" encoding="UTF-8"?>
	<Track version="2.0" xmlns="http://www.hikvision.com/ver20/XMLSchema">
		<id>101</id>
		<Channel>101</Channel>
		<Enable>true</Enable>
		<Description>trackType=standard,trackType=video,codecType=H.264-BP,resolution=1280x720,framerate=1.960000 fps,bitrate=2048 kbps</Description>
		<TrackGUID>e32e6863-ea5e-4ee4-997e-c42f90727eb3</TrackGUID>
		<Duration>P30DT0H</Duration>
		<DefaultRecordingMode>CMR</DefaultRecordingMode>
		<LoopEnable>false</LoopEnable>
		<SrcDescriptor>
			<SrcGUID>e32e6863-ea5e-4ee4-997e-c42f90727eb3</SrcGUID>
			<SrcChannel>1</SrcChannel>
			<StreamHint>trackType=standard,trackType=video,codecType=H.264-BP,resolution=1280x720,framerate=1.960000 fps,bitrate=2048 kbps</StreamHint>
			<SrcDriver>RTSP</SrcDriver>
			<SrcType>H.264-BP</SrcType>
			<SrcUrl>rtsp://localhost/PSIA/Streaming/channels/101</SrcUrl>
			<SrcType>DESCRIBE, SETUP, PLAY, TEARDOWN</SrcType>
			<SrcLogin>admin</SrcLogin>
		</SrcDescriptor>
		<TrackSchedule xmlns="">
			<ScheduleBlock>
				<ScheduleBlockGUID>{00000000-0000-0000-0000-000000000000}</ScheduleBlockGUID>
				<ScheduleBlockType>www.std-cgi.com/racm/schedule/ver10</ScheduleBlockType>
				<ScheduleAction>
					<id>1</id>
					<ScheduleActionStartTime>
						<DayOfWeek>%s</DayOfWeek>
						<TimeOfDay>%s</TimeOfDay>
					</ScheduleActionStartTime>
					<ScheduleActionEndTime>
						<DayOfWeek>%s</DayOfWeek>
						<TimeOfDay>%s</TimeOfDay>
					</ScheduleActionEndTime>
					<ScheduleDSTEnable>false</ScheduleDSTEnable>
					<Description>nothing</Description>
					<Actions>
						<Record>true</Record>
						<ActionRecordingMode>CMR</ActionRecordingMode>
					</Actions>
				</ScheduleAction>
			</ScheduleBlock>
		</TrackSchedule>
		<CustomExtensionList>
			<CustomExtension>
				<CustomExtensionName>www.hikvision.com/RaCM/trackExt/ver10</CustomExtensionName>
				<enableSchedule>true</enableSchedule>
				<SaveAudio>true</SaveAudio>
				<RedundancyRecord>false</RedundancyRecord>
				<PreRecordTimeSeconds>0</PreRecordTimeSeconds>
				<PostRecordTimeSeconds>0</PostRecordTimeSeconds>
			</CustomExtension>
		</CustomExtensionList>
	</Track>
	""" % (dayOfWeek, startTime, dayOfWeek, endTime)
	r = httpRequest("PUT", "/ISAPI/ContentMgmt/record/tracks/101", body)
	if r.status == 200:
		log("Set record plan succeed. [%s %s ~ %s]" % (dayOfWeek, startTime, endTime))
	else:
		log("Set record plan failed. [%s]" % r.reason)
		sys.exit(-1)


def scheduleOnce(startTime, endTime):
	t1 = time.localtime(startTime)
	t2 = time.localtime(endTime)
	startWeekDay = time.strftime("%A", t1)
	startTime = time.strftime("%H:%M:%S", t1)
	endWeekDay = time.strftime("%A", t2)
	endTime = time.strftime("%H:%M:%S", t2)
	if startWeekDay == endWeekDay:
		record(startWeekDay, startTime, endTime)
	else:
		log("Will set two record plans")
		record(startWeekDay, startTime, "24:00:00")
		record(endWeekDay, "00:00:00", endTime)
	return t2

if __name__ == "__main__":
	if len(sys.argv) != 3:
		usage()
	m1 = re.compile("(\w+):(\w+)@(.*)").match(sys.argv[1])
	m2 = re.compile("\d+").match(sys.argv[2])
	if m1 == None or m2 == None:
		usage()
	_username= m1.group(1)
	_password = m1.group(2)
	_host = m1.group(3)
	duration = float(m2.group(0))*60
	while True:
		t1 = deviceLocalTime()
		t2 = t1 + duration - int(t1)%60
		scheduleOnce(t1, t2)
		time.sleep(t2-t1)
		while deviceLocalTime() < t2 + 1:
			time.sleep(0.1)			

