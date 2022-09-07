#!/usr/bin/python

import sys
import httplib
import base64
import re

def usage():
	print '''
usage: {app} <command> [<command-args>]
<command>:
	search <start-time> <end-time>
	download <playback-uri>	<save-file>
	record <day-of-week> <start-time> <end-time>
examples:
	{app} record Saturday 15:10:00 15:15:00
	{app} search 2015-10-01T07:10:10Z 2015-10-10T07:15:00Z
	{app} download 'rtsp://10.69.3.129/Streaming/tracks/101?starttime=2015-10-10T07:10:10Z&amp;endtime=2015-10-10T07:15:00Z&amp;name=ch01_00000000000000000&amp;size=61485404' download.mp4
	'''.format(app=sys.argv[0])
	sys.exit(-1)

def httpRequest(method, uri, body=None):
	c = httplib.HTTPConnection("10.69.3.129")

	headers = {
		"Authorization": 'Basic %s' %  base64.b64encode(b"admin:admin123").decode("ascii"),
		"Connection": "keep-alive"
	}

	if body != None:
		headers["Content-Type"] = "applicaton/xml"
		headers["Content-Length"] = str(len(body))

	c.request(method, uri, body, headers)
	r = c.getresponse()
	#print r.status, r.reason 
	return r;

def record(dayOfWeek, startTime, endTime):
	body = '''
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
	''' % (dayOfWeek, startTime, dayOfWeek, endTime)
	r = httpRequest("PUT", "/ISAPI/ContentMgmt/record/tracks/101", body)
	if r.status == 200:
		print "Set record plan succeed."
	else:
		print r.status, r.reason
		

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
	if r.status == 200:
		bytes = r.read()
		#print bytes
		p = re.compile('<playbackURI>([^<]+)</playbackURI>')
		ms = p.finditer(bytes)
		for m in ms:
			print m.group(1)


def download(playbackURI, filename):
	body = '''
	<?xml version="1.0"?>
	<downloadRequest>
		<playbackURI>%s</playbackURI>
	</downloadRequest>
	''' % playbackURI 
	r = httpRequest("GET", "/ISAPI/ContentMgmt/download", body)
	if r.status != 200:
		print r.status, r.reason
		return
	f = open(filename, 'w')
	dowloaded = 0;
	while True:
		bytes = r.read(8192)
		if not bytes or len(bytes) == 0:
			print "\rdownload completed."
			f.close()
			break
		else:
			f.write(bytes)
			dowloaded += len(bytes)
			print "\rdownload %.1f M" % (dowloaded/(1024.0*1024.0)),
		
if __name__ == "__main__":
	if len(sys.argv) < 2:
		usage()
	if sys.argv[1] == "record" and len(sys.argv) == 5:
		record(sys.argv[2], sys.argv[3], sys.argv[4])
	elif sys.argv[1] == "search" and len(sys.argv) == 4:
		search(sys.argv[2], sys.argv[3])
	elif sys.argv[1] == "download" and len(sys.argv) == 4:
		download(sys.argv[2], sys.argv[3])	
	else:
		usage()
    
    

