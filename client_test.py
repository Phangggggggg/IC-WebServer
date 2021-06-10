import socket as sc
from time import sleep
a = 'youtube.com'
x = 'cs.muic.mahidol.ac.th'
y = 'localhost'
with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
    s.connect((y,11800))
    s.sendall(b'GET /index.html HTTP/1.1\r\n')
    # sleep(1)
    # s.sendall(b'/index.html HTTP/1.1\r')
    # sleep(0.5)
    s.sendall(b'Connection: close\r\n')
    s.sendall(b'\r\n')
    while data := s.recv(1024):
        print(data)


