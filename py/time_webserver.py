#coding=utf-8
from socket import *
import multiprocessing
import time

html = """
<html>
<body>
    <p>Client comes from %s</p>
    <p>Server time is %s</p>
</body>
</html>
"""

def application(environ, start_response):
    start_response('200 OK', [('Content-Type', 'text/html')])
    return html % (environ, time.strftime("%Y-%m-%d %H:%M:%S"))

class WebServer:
   def __init__(self):
       self.server_socket = socket(AF_INET, SOCK_STREAM)
       self.server_socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
       self.server_socket.bind(('', 1111))
       self.server_socket.listen(16)

   def start_http_service(self):
       while True:
           client_socket, client_addr = self.server_socket.accept()
           new_process = multiprocessing.Process(target=self.handle_client, args=(client_socket, client_addr))
           new_process.start()
           client_socket.close()

   def handle_client(self, client_socket, client_addr):
       request = client_socket.recv(1024).decode("utf-8")
       print("[request]:\n" +  request)
       environ = client_addr[0] + ":" + str(client_addr[1])
       response_body = application(environ, self.start_response)
       response_header = "HTTP/1.1 " + self.application_header[0] + "\r\n"
       for var in self.application_header[1]:
           response_header += var[0]+":"+var[1] + "\r\n"
       response_header += "\r\n"
       response = response_header + response_body
       print("[response]:\n" + response)
       client_socket.send(response.encode("utf-8"))

   def start_response(self, status, header):
       self.application_header = [status, header]

def main():
    webserver = WebServer()
    webserver.start_http_service()

if __name__ == "__main__":
    main()