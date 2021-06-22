import socket as sc
from time import sleep
import http.client

def legal_test1(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)

def legal_test2(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)

def legal_test3(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        s.sendall(b'Host: cs.muic.mahidol.ac.th\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)

def legal_test4(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD ')
        sleep(1);
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        s.sendall(b'Host: cs.muic.mahidol.ac.th\r\n')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)    

def legal_test5(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        sleep(1);
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        sleep(0.6);
        s.sendall(b'Host: google.com\r\n')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)    

def legal_test6(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        sleep(0.6);
        s.sendall(b'Host: cs.muic.mahidol.ac.th\r\n')
        s.sendall(b'Connection: closed\r\n')
        s.sendall(b'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 


def legal_test7(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        sleep(2)
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        sleep(0.6);
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 


def illegal_test1(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GETTY ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/1.1\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)

def illegal_test2(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD ')
        s.sendall(b'/ ')
        s.sendall(b'HTTP/2.0\r\n')
        while data := s.recv(1024):
            print(data)


def legal_test3(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'DELETE ')
        sleep(1);
        s.sendall(b'/index.html')
        s.sendall(b'HTTP/1.1\r\n')
        sleep(0.6);
        s.sendall(b'Host: xyz.com\r\n')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data)   

def legal_test4(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        s.sendall(b'/text.abc ')
        s.sendall(b'HTTP/1.1\t\n')
        sleep(0.6);
        s.sendall(b'Host: cs.muic.mahidol.ac.th\r\n')
        s.sendall(b'Connection: closed\t\n')
        s.sendall(b'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 

def legal_test5(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'GET ')
        sleep(1);
        s.sendall(b'/index.html')
        s.sendall(b'HTTP/1.\r\n')
        sleep(0.6);
        s.sendall(b'Host: xyz.com')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 

def legal_test6(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD ')
        s.sendall(b'/index.htm ')
        s.sendall(b'HTTP/1.\r\n\r\n')
        s.sendall(b'Host: xyz.com')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 

def legal_test7(host_name,port):
    with sc.socket(sc.AF_INET, sc.SOCK_STREAM) as s:
        s .connect((host_name,port))
        s.sendall(b'HEAD GET')
        s.sendall(b'/index.html ')
        s.sendall(b'HTTP/1.\r\n')
        s.sendall(b'Host: xyz.com')
        s.sendall(b'Connection: keep-alive\r\n')
        s.sendall(b'\r\n')
        while data := s.recv(1024):
            print(data) 