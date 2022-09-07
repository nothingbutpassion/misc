import os
import time
import socket
import threading

class CaptureImages:
    def __init__(self):
        self.images = []
        self.lock = threading.Lock()
    def get(self, index):
        with self.lock:
            return self.images[index]
    def len(self):
        with self.lock:
            return len(self.images)
    def append(self, other):
        print("add image: " + str(other))
        with self.lock:
            self.images.append(other)
            self.images.sort(key=lambda x: x["mtime"])


class WorkerThread(threading.Thread):
    def __init__(self, client_socket, client_address, capture_images):
        threading.Thread.__init__(self)
        self.client_socket = client_socket
        self.client_address = client_address
        self.capture_images = capture_images

    def run(self):
        self.handle_client(self.client_socket, self.client_address)

    def handle_client(self, client_socket, client_address):
        print("[connection]: " + str(client_address))
        try:
            data = client_socket.recv(4096)
            if data:
                self.handle_request(data, client_socket)
        except Exception as e:
            print("socket exception: " + str(e))
        client_socket.close()

    def handle_request(self, data, client_socket):
        """
        MJPeg over HTTP

        see https://stackoverflow.com/questions/47729941/mjpeg-over-http-specification
        """
        request = data.decode("utf-8")
        print("[request]: \n%s" % request)
        lines = request.splitlines()
        method, path, version = lines[0].split()
        # request_header = {}
        # for line in lines[1:]:
        #     p = line.find(":")
        #     request_header[line[:p].strip()] = line[p+1:].strip()
        if method == "GET":
            self.handle_get(client_socket, path)
        elif method == "POST":
            self.handle_post(client_socket, path)
        else:
            client_socket.send("HTTP/1.1 400 Bad Request")

    def handle_get(self, client_socket, path):
        # send response header
        response_header = "HTTP/1.1 200 OK\r\n"
        response_header += "Content-Type: multipart/x-mixed-replace; boundary=--This_is_a_boundary_for_each_jpeg_image\r\n"
        response_header = response_header.encode("utf-8")
        print("[response]: \n%s" % response_header)
        client_socket.send(response_header)

        # send response body
        count = 0
        while count < 11:
            jpeg = self.get_jpeg_frame(path)
            if jpeg is None:
                break
            response = "\r\n--This_is_a_boundary_for_each_jpeg_image\r\n"
            response += "Content-Type: image/jpeg\r\n\r\n"
            response = response.encode("utf-8") + jpeg
            print("[response]: \n%d bytes jpeg data" % len(response))
            client_socket.send(response)
            count += 1
        response = "\r\n--This_is_a_boundary_for_each_jpeg_image--"
        response = response.encode("utf-8")
        print("[response]: \n%s" % response)
        client_socket.send(response)

    def get_jpeg_frame(self, path):
        raw = None
        num_images = self.capture_images.len()
        if num_images > 0:
            image_index = num_images - 1;
            pos = path.rfind("/")
            if path[pos+1:]:
                image_index = int(path[pos+1:])
            if image_index < -num_images or image_index >= num_images:
                image_index = num_images - 1;
            elif image_index < 0:
                image_index += num_images
            with open(self.capture_images.get(image_index)["path"], "rb") as f:
                raw = f.read()
        return raw

    def handle_post(self, client_socket, path):
        response = "HTTP/1.1 200 OK\r\n\r\n<html><body><p>JUST FOR TESTING</p></body></html>"
        client_socket.send(response.encode("utf-8"))

class HttpServer:
    def __init__(self, port, capture_images):
        self.capture_images = capture_images
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('', port))
        self.socket.listen(16)

    def start(self):
        while True:
            client_socket, client_address = self.socket.accept()
            worker_thread = WorkerThread(client_socket, client_address, capture_images)
            worker_thread.start()


def file_scanning(upload_dir, capture_dir, capture_images):
    while True:
        images = []
        upload_files = [f for f in os.listdir(upload_dir)]
        for f in upload_files:
            src_path = os.path.join(upload_dir, f)
            dst_path = os.path.join(capture_dir, f)
            if f.endswith(".jpg") or f.endswith(".JPG"):
                os.rename(src_path, dst_path)
                images.append({"path":dst_path, "mtime": os.path.getmtime(dst_path)})
            else:
                os.remove(src_path)
        if images:
            for image in images:
                print("fond image: " + str(image))
                capture_images.append(image)
        time.sleep(1)


if __name__ == "__main__":
    root_dir = os.path.dirname(os.path.realpath(__file__))
    upload_dir = os.path.join(root_dir, "upload")
    capture_dir = os.path.join(root_dir, "capture")
    capture_images = CaptureImages()
    capture_files = [os.path.join(capture_dir, f) for f in os.listdir(capture_dir) if f.lower().endswith(".jpg")]
    for f in capture_files:
        capture_images.append({"path": f, "mtime": os.path.getmtime(f)})
    scanner = threading.Thread(target=file_scanning, args=(upload_dir, capture_dir, capture_images))
    scanner.start()
    server = HttpServer(1111, capture_images)
    server.start()









