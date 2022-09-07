import os
import sys
import time


def start_capture_upload(internal=60):
    import cv2
    vc = cv2.VideoCapture(0)
    while True:
        ok, img = vc.read()
        if not ok:
            print("read video frame failed")
            break
        ok, jpeg = cv2.imencode(".jpg", img)
        filename = time.strftime("%Y%m%d%H%M%S.jpg")
        with open(filename, "wb") as f:
            f.write(jpeg)
        if os.path.isfile(filename):
            print("uploading %s" % filename)
            cmd = "sshpass -p yuhao123 scp -P 29723 %s root@172.96.234.161:/root/upload/" % filename
            if os.system(cmd) != 0:
                print('execute shell command "%s" failed' % cmd)
            else:
                print("uploaded  %s" % filename)
            os.remove(filename)
        time.sleep(internal)

def start_caputure(outdir, internal=1):
    import cv2
    vc = cv2.VideoCapture(0)
    while True:
        ok, img = vc.read()
        if not ok:
            print("read video frame failed")
            break;
        ok, jpeg = cv2.imencode(".jpg", img)
        filename = outdir + "/" + time.strftime("%Y%m%d%H%M%S.jpg")
        filename = os.path.join(outdir, filename)
        with open(filename, "wb") as f:
            f.write(jpeg)
        time.sleep(internal)

def start_upload(outdir, internal=1):
    while True:
        paths = [os.path.join(outdir, f) for f in os.listdir(outdir) if f.lower().endswith(".jpg")]
        images = [(p, os.path.getmtime(p)) for p in paths]
        images.sort(key=lambda x:x[1])
        for image in images:
            print("uploading %s" % image[0])
            cmd = "sshpass -p yuhao123 scp -P 29723 %s root@172.96.234.161:/root/upload/" % image[0]
            if os.system(cmd) != 0:
                print('execute shell command "%s" failed'% cmd)
            else:
                print("uploaded  %s" % image[0])
                os.remove(image[0])
        time.sleep(internal)

if __name__ == "__main__":
    if len(sys.argv) !=1 and len(sys.argv) != 3:
        print("Usage: %s [capture|upload  <image-dir>]" % sys.argv[0])
        sys.exit(-1)
    if len(sys.argv) == 1:
        start_capture_upload()
    else:
        outdir = sys.argv[2]
        if sys.argv[1] == "capture":
            start_caputure(outdir)
        elif sys.argv[1] == "upload":
            start_upload(outdir)