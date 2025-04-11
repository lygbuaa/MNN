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

        # self.dst_addr = ("192.168.1.102", 10000)
        if type == socket.SOCK_DGRAM:
            self.socket_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket_server.bind((host, port))
        elif type == socket.SOCK_STREAM:
            self.socket_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket_server.bind((host, port))
            self.socket_server.listen()
            (self.conn, self.client_addr) = self.socket_server.accept()
            print("accept client address: {}".format(self.client_addr))
        else:
            raise SystemError("socket type {} not supported !".format(type))
        
        GfCheckSocketBuffer(self.socket_server)
        self.socket_server.setblocking(True)
        print("udp server listen on {}:{}".format(self.host, self.port))

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
        if self.counter % 1 == 0:
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
    def __init__(self, host="127.0.0.1", port=2369, type=socket.SOCK_STREAM):
        self.host = host
        self.port = port
        self.dst_addr = (host, port)
        self.type = type
        self.data_len = 3 ### 3 bytes for each chinese character.
        self.counter = 0
        self.msg = bytearray()
        self.full_resp = bytearray()

        if type == socket.SOCK_DGRAM:
            self.socket_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #SOCK_STREAM
            GfCheckSocketBuffer(self.socket_client)
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
        self.full_resp = bytearray()

    def recv_msg(self):
        if self.type == socket.SOCK_DGRAM:
            self.msg, addr = self.socket_client.recvfrom(self.data_len)
        elif self.type == socket.SOCK_STREAM:
            self.msg = self.socket_client.recv(self.data_len)
        else:
            raise SystemError("socket type {} not supported !".format(type))

        if not self.msg:
            print("recv error, loop quit")
            # self.th._stop()
            sys.exit(1)
        recvtime = time.time()
        if self.counter % 1 == 0:
            print("recv [{}] msg from server, timestamp {}sec".format(self.counter, recvtime))
        self.counter += 1

    def post_process(self, msg):
        # print("this is SocketServer base stub!")
        msg_str = str(self.msg, encoding='utf-8')
        print("recv msg_str: {}".format(msg_str))
        self.full_resp += self.msg

    def start_recv_thread(self, run_background=True):
        try: 
            self.th = ThreadBase(name="recv_thread", preprocess=self.recv_msg, callback=self.post_process, callback_args=(self.msg,))
            self.th.setDaemon(run_background)
            self.th.start()
            print("recv thread start")
        except Exception as err:
            print("start_recv_thread error: {}".format(err))

def make_request_toggle_model(idx:int=0) -> bytearray:
    toggle_model_json = {"toggle_model": 0}
    toggle_model_json["toggle_model"] = idx
    msg = bytearray(1024) #1024
    msg.__init__(len(msg))
    msg_str = bytearray(json.dumps(toggle_model_json), "utf-8")
    msg[0:len(msg_str)] = msg_str
    print("toggle_model: {}".format(toggle_model_json["toggle_model"]))
    print("make request: {}".format(msg_str))
    return msg

def make_request_prompt(prompt:str=u"") -> bytearray:
    prompt_json = {"prompt": u""}
    prompt_json["prompt"] = prompt
    msg = bytearray(1024) #1024
    msg.__init__(len(msg))
    msg_str = bytearray(json.dumps(prompt_json), "utf-8")
    msg[0:len(msg_str)] = msg_str
    print("prompt: {}".format(prompt_json["prompt"]))
    print("make request: {}".format(msg_str))
    return msg

def make_request_prompt_with_image(image_filename:str="", prompt:str="") -> bytearray:
    image_prompt_json = {"image_filename": "wuhan_bridge.jpg", "height": 270, "width": 480, "prompt": u"图片中是哪里?"}
    image_prompt_json["image_filename"] = image_filename
    image_prompt_json["prompt"] = prompt
    msg = bytearray(1024) #1024
    msg.__init__(len(msg))
    msg_str = bytearray(json.dumps(image_prompt_json), "utf-8")
    msg[0:len(msg_str)] = msg_str
    print("image_filename: {}, prompt: {}".format(image_prompt_json["image_filename"], image_prompt_json["prompt"]))
    print("make request: {}".format(msg_str))
    return msg


def run_client():
    s_client = SocketClient(host="127.0.0.1", port=10001, type=socket.SOCK_STREAM)
    s_client.start_recv_thread()
    ### answer: B, https://huggingface.co/datasets/MMMU/MMMU/viewer/Chemistry
    image1_path_json = {"image_filename": "mmmu/dev_Chemistry_3.jpg", "height": 270, "width": 480, "prompt": u"In this image, how many molecules of the sweetener saccharin can be prepared from 30 $C$ atoms, 25 $H$ atoms, 12 $O$ atoms, 8 $S$ atoms, and 14 $N$ atoms? Pick the correct option from ['3', '4', '5', '6']."}
    ### answer: A, https://huggingface.co/datasets/MMMU/MMMU/viewer/Computer_Science
    image2_path_json = {"image_filename": "mmmu/dev_Computer_Science_2.jpg", "height": 270, "width": 480, "prompt": u"This image shows the truth table and calculation circuit diagram for the addition of 1-digit integers. What is the simple circuit that the diagram represents usually called? Pick the correct option from ['half adder', 'full adder']."}
    ### answer: D
    image3_path_json = {"image_filename": "mmmu/dev_Computer_Science_3.jpg", "height": 270, "width": 480, "prompt": u"How many context switches are required in the last row of this image? Pick the correct option from ['0', '1', '2', 'None of the other answers']."}
    ### answer: A
    image4_path_json = {"image_filename": "mmmu/dev_Computer_Science_4.jpg", "height": 270, "width": 480, "prompt": u"This image is Typical details of the upstream and downstream channels in North America. .Are downstream channels and upstream channels managed in the same way? Pick the correct option from ['same', 'not the same']."}
    ### answer: A, https://huggingface.co/datasets/MMMU/MMMU/viewer/Economics
    image5_path_json = {"image_filename": "mmmu/dev_Economics_2.png", "height": 270, "width": 480, "prompt": u"In this image, The table below describes production possibilities for Mexico and Nigeria. Each number in the table shows the number of workers needed to produce one unit of the product. Suppose that both countries have 90 workers. What is the opportunity cost of producing Glasses in Mexico? Pick the correct option from ['1.20 Shoes', '1.00 Shoes', '0.83 Shoes', '0.75 Shoes']."}
    # image6_path_json = {"image_filename": "road_uturn.jpg", "height": 270, "width": 480, "prompt": u"图中这条道路可以左转吗?"}
    # image7_path_json = {"image_filename": "road_work.jpg", "height": 270, "width": 480, "prompt": u"为图中这条道路规划行车路线."}
    # image8_path_json = {"image_filename": "road_crash.jpg", "height": 270, "width": 480, "prompt": u"为图中这条道路规划行车路线?"}
    # image9_path_json = {"image_filename": "wuhan_bridge.jpg", "height": 270, "width": 480, "prompt": u"图片中是哪个地方?"}
    # image10_path_json = {"image_filename": "road_night.jpg", "height": 270, "width": 480, "prompt": u"为图中这条道路规划行车路线?"}

    image_json_list = [
        image1_path_json,
        image2_path_json,
        image3_path_json,
        image4_path_json,
        image5_path_json,
        # image6_path_json,
        # image7_path_json,
        # image8_path_json,
        # image9_path_json,
        # image10_path_json
    ]

    msg = make_request_toggle_model(idx=1)
    s_client.send_array(msg)
    time.sleep(20.0)
    print("response: {}".format(str(s_client.full_resp, encoding='utf-8')))

    for img_json in image_json_list:
        msg = make_request_prompt_with_image(img_json["image_filename"], img_json["prompt"])
        s_client.send_array(msg)
        time.sleep(120.0)
        print("response: {}".format(str(s_client.full_resp, encoding='utf-8')))


if __name__ == "__main__":
    print("********** running test with MMMU (Massive Multi-discipline Multimodal Understanding) ************")
    run_client()