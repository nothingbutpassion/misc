import sys
import signal
import cv2

interrupted = False
def signal_handler(signum, frame):
    global interrupted
    interrupted = True

def record(filename):
	vc = cv2.VideoCapture(0)
	if vc.isOpened():
		size = int(vc.get(cv2.CAP_PROP_FRAME_WIDTH)), int(vc.get(cv2.CAP_PROP_FRAME_HEIGHT))
		fps = vc.get(cv2.CAP_PROP_FPS)
		format = cv2.VideoWriter.fourcc('m', 'p', '4', 'v')
		vw = cv2.VideoWriter(filename, format, fps, size, True)
		if vw.isOpened() :
			print("Press any key to exit")
			while not interrupted:
				ok, img = vc.read()
				vw.write(img)
				cv2.imshow("camera", img)
				if cv2.waitKey(11) > 0:
					break
			vw.release()
		else:
			print(f"create {filename} failed")
	else:
		print("open video capture device failed")

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Usage: {sys.argv[0]} <output-mp4-file>")
		sys.exit(-1)
	signal.signal(signal.SIGINT, signal_handler)
	record(sys.argv[1])
