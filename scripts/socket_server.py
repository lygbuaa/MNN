#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import time, threading, socket, struct, sys, json
# import numpy as np

def GfCheckSocketBuffer(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024*1024)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024*1024)
    rcvbuf = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    sndbuf = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    print("rcvbuf: {}, sndbuf: {}".format(rcvbuf, sndbuf))

class ThreadBase(threading.Thread):
    def __init__(self, callback=None, callback_args=None, *args, **kwargs):
        self.preprocess = kwargs.pop('preprocess')
        super(ThreadBase, self).__init__(target=self.loop_with_callback, *args, **kwargs)
        self.callback = callback
        self.callback_args = callback_args

    def loop_with_callback(self):
        while True:
            if self.preprocess is not None:
                self.preprocess()
            if self.callback is not None:
                self.callback(*self.callback_args)

class SocketServer(object):
    def __init__(self, host="127.0.0.1", port=10001, len=1024, type=socket.SOCK_STREAM):
        self.host = host
        self.port = port
        self.data_len = len
        self.msg = None
        self.counter = 0
        self.conn = None
        self.type = type
        self.client_addr = None
        self.dst_addr = (host, port)

        if type == socket.SOCK_DGRAM:
            self.socket_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket_server.bind((host, port))
        elif type == socket.SOCK_STREAM:
            self.socket_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket_server.bind((host, port))
            self.socket_server.listen()
            (self.conn, self.client_addr) = self.socket_server.accept()
            print("accept client address: {}".format(self.client_addr))
        else:
            raise SystemError("socket type {} not supported !".format(type))
        
        GfCheckSocketBuffer(self.socket_server)
        self.socket_server.setblocking(True)
        print("udp server listen on {}:{}".format(self.host, self.port))

    def send_array(self, data_array):
        if self.type == socket.SOCK_DGRAM:
            self.socket_server.sendto(data_array, self.dst_addr)
        elif self.type == socket.SOCK_STREAM:
            self.conn.send(data_array)
        else:
            raise SystemError("socket type {} not supported !".format(type))

    def recv_msg(self):
        if self.type == socket.SOCK_DGRAM:
            self.msg, addr = self.socket_server.recvfrom(self.data_len)
        elif self.type == socket.SOCK_STREAM:
            self.msg = self.conn.recv(self.data_len)
            addr = self.client_addr
        else:
            raise SystemError("socket type {} not supported !".format(type))

        if not self.msg:
            print("recv error, loop quit")
            # self.th._stop()
            sys.exit(1)
        recvtime = time.time()
        if self.counter % 1024 == 0:
            print("recv [{}] msg from client {}, timestamp {}sec".format(self.counter, addr, recvtime))
        self.counter += 1

    def post_process(self, msg):
        print("this is SocketServer base stub!")

    def start_recv_thread(self, run_background=True):
        try: 
            self.th = ThreadBase(name="recv_thread", preprocess=self.recv_msg, callback=self.post_process, callback_args=(self.msg,))
            self.th.setDaemon(run_background)
            self.th.start()
            print("recv thread start")
        except Exception as err:
            print("start_recv_thread error: {}".format(err))

class SocketClient(object):
    def __init__(self, host="127.0.0.1", port=2369, type=socket.SOCK_DGRAM):
        self.host = host
        self.port = port
        self.dst_addr = (host, port)

        if type == socket.SOCK_DGRAM:
            self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #SOCK_STREAM
            GfCheckSocketBuffer(self.socket_client)
            # self.socket_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        elif type == socket.SOCK_STREAM:
            self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            GfCheckSocketBuffer(self.socket_client)
            self.socket_client.connect((host, port))
            print("client connect to {}:{}".format(host, port))
        else:
            raise SystemError("socket type {} not supported !".format(type))

        print("udp server will send to {}:{}".format(self.host, self.port))

    def send_list(self, data_list=[]):
        msg = bytearray(data_list)
        print("send msg: {}".format(msg))
        self.socket_client.sendto(msg, self.dst_addr)

    def send_array(self, data_array):
        self.socket_client.sendto(data_array, self.dst_addr)

def run_server():
    class SocketServerQwen(SocketServer):
        def __init__(self, host="127.0.0.1", port=10001, len=1024, type=socket.SOCK_STREAM):
            super(SocketServerQwen, self).__init__(host=host, port=port, len=len, type=type)

        # the msg passed in is not used
        def post_process(self, msg):
            # print("this is SocketServerQwen post process")
            if self.msg is not None:
                # header = self.msg[0:3]
                # print("msg len: {}".format(len(self.msg)))
                # counter = struct.unpack(">I", self.msg[0:4])[0]
                end_idx = self.msg.find(b'\x00')
                print("recv bytes: {}, end_idx: {}".format(len(self.msg), end_idx))
                if len(self.msg) < 2:
                    print("empty msg!")
                    return
                if end_idx == -1:
                    end_idx = len(self.msg)

                valid_str = self.msg[:end_idx].decode('utf-8')
                print("recv msg({}): {}".format(len(valid_str), valid_str))
                req = json.loads(valid_str)
                if "image_filename" in req and "prompt" in req:
                    print("image_filename: {}, prompt: {}".format(req["image_filename"], req["prompt"]))
                elif "prompt" in req:
                    print("prompt: {}".format(req["prompt"]))
                resp_bytes = bytearray(u"这是，我的回复。。。", "utf-8")
                time.sleep(1)
                self.send_array(resp_bytes)

    s_server = SocketServerQwen(host="192.18.34.101", port=10001, len=1024, type=socket.SOCK_STREAM)
    s_server.start_recv_thread(run_background=True)
    counter = 0
    while True:
        time.sleep(1.0)

if __name__ == "__main__":
    print("********** running test ************")
    run_server()