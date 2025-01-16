#!/usr/bin/env python3

'''
Basic IPv4 router (static routing) in Python.
'''

import time
import switchyard
from switchyard.lib.userlib import *
# show ARP Table information
def showArpTable(table):
    print("----ip addr----|----MAC addr----")
    for k, [v1, v2] in table.items():
        print(f"{k}        {v1}")
    print("--------------------------------\n")


class Router(object):
    def __init__(self, net: switchyard.llnetbase.LLNetBase):
        self.net = net
        self.interfaces = self.net.interfaces()
        self.arpTable = {}
        

    def handle_packet(self, recv: switchyard.llnetbase.ReceivedPacket):
        timestamp, ifaceName, packet = recv

        #timeout
        for k, [v1, v2] in self.arpTable.items():
            if time.time() - v2 >= 727:
                self.arpTable.pop(k)

        # TODO: your logic here
        # 判断是否为ARP包 
        print("before:")
        showArpTable(self.arpTable)
        arp = packet.get_header(Arp)
        if arp:
            if arp._operation == 1:
                print("This packet is an ARP request packet.")
                my_senderhwaddr = arp.senderhwaddr
                my_senderprotoaddr = arp.senderprotoaddr
                my_targetprotoaddr = arp.targetprotoaddr
                self.arpTable[my_senderprotoaddr] = [my_senderhwaddr, time.time()]
                #判断targetprotoaddr是否为分配给路由器某个接口的IP地址
                #if my_targetprotoaddr in self.arpTable:
                #    replyPkt = create_ip_arp_reply(self.arpTable[my_targetprotoaddr], my_senderhwaddr, my_targetprotoaddr, my_senderprotoaddr)
                #    self.net.send_packet(ifaceName, replyPkt)
                #else:    
                for interf in self.interfaces:
                    if interf.ipaddr == my_targetprotoaddr:    #目标 IP 地址分配给了路由器的某个接口
                        self.arpTable[my_targetprotoaddr] = [interf.ethaddr, time.time()]
                        replyPkt = create_ip_arp_reply(interf.ethaddr, my_senderhwaddr, my_targetprotoaddr, my_senderprotoaddr)
                        self.net.send_packet(ifaceName, replyPkt)
            else:
                print("This is an ARP reply packet")
        else:
            #现阶段:丢弃
            print("This packet is not an ARP packet.")
        print("after")
        showArpTable(self.arpTable)


    def start(self):
        '''A running daemon of the router.
        Receive packets until the end of time.
        '''
        while True:
            try:
                recv = self.net.recv_packet(timeout=1.0)
            except NoPackets:
                continue
            except Shutdown:
                break

            self.handle_packet(recv)

        self.stop()

    def stop(self):
        self.net.shutdown()


def main(net):
    '''
    Main entry point for router.  Just create Router
    object and get it going.
    '''
    router = Router(net)
    router.start()
